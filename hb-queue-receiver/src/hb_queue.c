#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>		/* for NF_ACCEPT */
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
#include <signal.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#include "hashtable.h"
#include "list.h"

#include "utility.h"
#include "hb_config.h"
#include "drop.h"

//#define S2S
#define DROP

static struct nfq_handle *h;
static struct nfq_q_handle *qh;

static hash_table epoch_list_ht;
static pthread_mutex_t epoch_list_ht_lock;

static long base_time;
static long expiration_interval;
static pthread_t expirator_tid;
static int running;

#ifdef S2S
#define ARRAY_SIZE 100
static int self_sockfd;
static hash_table epoch_sem_ht;
static pthread_mutex_t epoch_sem_ht_lock;
static int received_self_msg_array[ARRAY_SIZE][IRQ_NUM]; 
// recond the round so that clear is not needed for new round
static int array_round = 1; 
#endif

static void destroy_nfqueue() {
	printf("unbinding from queue 0\n");
	nfq_destroy_queue(qh);

#ifdef INSANE
	/* normally, applications SHOULD NOT issue this command, since
	 * it detaches other programs/sockets from AF_INET, too ! */
	printf("unbinding from AF_INET\n");
	nfq_unbind_pf(h, AF_INET);
#endif

	printf("closing library handle\n");
	nfq_close(h);
}

static void signal_handler(int signo) {
        running = 0;
        // destroy nfqueue in signal handler so that recv won't be blocked.
        destroy_nfqueue();
        printf("Singal handled by switching running to 0 and destroying nfqueue.\n");
}

static void clear_all() {
        printf("Destroy epoch_list_ht.\n");
        ht_destroy(&epoch_list_ht);
        
#ifdef S2S
        printf("Destroy epoch_sem_ht.\n");
        ht_destroy(&epoch_sem_ht);
        
        printf("Close self_sockfd.\n");
        close(self_sockfd);
#endif
        
        printf("Cancel thread expirator_tid.\n");
        pthread_cancel(expirator_tid);
}

// return epoch id
long round_to_epoch(long time) {
        time -= base_time;
        return time < 0 ? -1 : (long)(time / expiration_interval + 1);
}

long round_to_interval(long id) {
        return base_time + (id * expiration_interval);
}

/*
 * Process packet ands return packet id 
 */
static u_int32_t process_hb(struct nfq_data *tb) {
        int id = 0;
        struct nfqnl_msg_packet_hdr *ph;
	int payload_len;
	unsigned char *data;
        struct iphdr* iph;
        char saddr[INET_ADDRSTRLEN];

        ph = nfq_get_msg_packet_hdr(tb);
        if(ph)
                id = ntohl(ph->packet_id);
        
        payload_len = nfq_get_payload(tb, &data);
        if(payload_len >= 0) {                
                iph = (struct iphdr *) data;
                inet_ntop(AF_INET, &(iph->saddr), saddr, INET_ADDRSTRLEN);
        }

#ifdef S2S
        /*
         * Packets sent from SELF_IP are send-to-self messages.
         * The content of this TCP packet, beginning at the 52th byte, consists of two 8-byte longs.
         * The first long contains epoch_id, while the seocnd long contains ring_id. 
         */
        if(strcmp(saddr, SELF_IP) == 0) {
                long *content = (long *)(data + 52);
                long epoch_id  = *content; 
                long ring_id  = *(content + 1); 
                int i;
                size_t value_size;
                
                // get array index; if it's a new round, increase round
                int array_index = epoch_id % ARRAY_SIZE;
                
                if(received_self_msg_array[array_index][ring_id] < array_round) {
                        received_self_msg_array[array_index][ring_id] = array_round; 
                } else { // the S2S message for the ring is redundant
                        goto ret;
                }
                        
                for(i=0; i<IRQ_NUM; i++) {
                        if(received_self_msg_array[array_index][i] != array_round)
                                goto ret;
                }
                
                printf("[S2S] All S2S messages for Epoch %ld are received.\n", epoch_id);

                // reach the end of a round
                if(array_index == (ARRAY_SIZE -1))
                        array_round ++;    
                
                pthread_mutex_lock(&epoch_sem_ht_lock); 
                sem_t **sem = (sem_t**) ht_get(&epoch_sem_ht, &epoch_id, sizeof(long), &value_size);
                if(sem != NULL) {
                        sem_post(*sem);
                        printf("[S2S] The semaphore for Epoch %ld is posted.\n", epoch_id);
                } else 
                        printf("[S2S] Error: The semaphore for Epoch %ld not found!\n", epoch_id);
                pthread_mutex_unlock(&epoch_sem_ht_lock);
        }
#endif
        /*
         * Packets sent not from SELF_IP are heartbeat messages.
         * The content of this UDP packet, beginning at the 28th byte, consists of one 8-byte long.
         * The long contains send_time. 
         */
        if(strcmp(saddr, SELF_IP) != 0) {
                long *content = (long *)(data + 28);
                long send_time = *content;
                long epoch_id = round_to_epoch(send_time); // get the epoch id by sending time
                size_t value_size;
                list_t **ip_list;

                printf("[Receiver] Heartbeat from Ip %s for Epoch (%ld, %ld) sent at %ld received at %ld.\n", \
                        saddr, epoch_id, round_to_interval(epoch_id), send_time, now());
                
                if(epoch_id < 0)
                        goto ret;

                pthread_mutex_lock(&epoch_list_ht_lock); 
                
                // Touch the node for the epoch
                ip_list = (list_t**) ht_get(&epoch_list_ht, &epoch_id, sizeof(long), &value_size);
                if(ip_list != NULL) {
                        list_node_t *ip_to_remove = list_find(*ip_list, saddr);
                        if(ip_to_remove != NULL) {
                                list_remove(*ip_list, ip_to_remove);
                                printf("[Receiver] Removed Ip %s for Epoch %ld.\n", saddr, epoch_id);
                        } 
                }
                
                // Insert Ip into the Ip list of the next epoch
                epoch_id ++;
                
                ip_list = (list_t**) ht_get(&epoch_list_ht, &epoch_id, sizeof(long), &value_size);
                if(ip_list == NULL) {
                        list_t *tmp = list_new();
                        tmp->match = ip_equal;
                        tmp->free = free_val;
                        ip_list = &tmp;
                        ht_insert(&epoch_list_ht, &epoch_id, sizeof(long), ip_list, sizeof(list_t*));
                        printf("[Receiver] Created and inserted the Ip list for Epoch %ld.\n", epoch_id);
                }
                
                if(list_find(*ip_list, saddr) == NULL) {
                        // Allocate the memory of _saddr in heap
                        char *_saddr = (char*) calloc(INET_ADDRSTRLEN, sizeof(char));
                        strcpy(_saddr, saddr);
                        list_rpush(*ip_list, list_node_new(_saddr));
                }

                pthread_mutex_unlock(&epoch_list_ht_lock); 
        } 

ret:
        return id;
}

/*
 * Thread for expiration check
 */
void *expirator(void *arg) {
        long next_epoch_id;

#ifdef DROP
        struct kernel_drop_stats last_stats;
        init_kernel_drop(&last_stats);
#endif
        
        base_time = now();
        next_epoch_id = round_to_epoch(now()); 
        printf("[Expirator] Thread started with base_time = %ld, expiration_interval = %ld, next_epoch_id = %ld\n", \
                base_time, expiration_interval, next_epoch_id);
        
        while(running) {
                long current_time = now();
                long next_expiration_time = round_to_interval(next_epoch_id);
                list_t **ip_list;
                size_t value_size;

                if(next_expiration_time > current_time) {
                        struct timespec sleep_ts = sleep_time(next_expiration_time - current_time);
                        nanosleep(&sleep_ts, NULL);
                        continue;
                }  

#ifdef S2S
                // Allocate and initialize the memory of a new semaphore.
                sem_t *sem_p = (sem_t*) calloc(1, sizeof(sem_t));
                sem_init(sem_p, 0, 0);

                // Insert semaphore into sem hashtable.
                pthread_mutex_lock(&epoch_sem_ht_lock); 
                ht_insert(&epoch_sem_ht, &next_epoch_id, sizeof(long), &sem_p, sizeof(sem_t*));
                pthread_mutex_unlock(&epoch_sem_ht_lock); 
                printf("\t[S2S] Semaphore for Epoch %ld inserted.\n", next_epoch_id);
                
                // Send a TCP message containing next_epoch_id to Self Sender.
                if((send(self_sockfd, &next_epoch_id, MSGSIZE, 0)) != MSGSIZE) {
                        fprintf(stderr, "Error: Send to self wrong!\n");
                        pthread_mutex_unlock(&epoch_sem_ht_lock); 
                        break;
                }
                printf("\t[S2S] Self-message for Epoch %ld sent at time %ld.\n", next_epoch_id, now());
                
                // Wait sem post from S2S message receiver.
                sem_wait(sem_p); 

                // Semaphore is posted.
                pthread_mutex_lock(&epoch_sem_ht_lock); 
                ht_remove(&epoch_sem_ht, &next_epoch_id, sizeof(long));
                free(sem_p);
                printf("\t[S2S] Semaphore for Epoch %ld removed.\n", next_epoch_id); 
                pthread_mutex_unlock(&epoch_sem_ht_lock); 
#endif

#ifdef DROP
                // Determine if there is packet dropped 
                if(check_kernel_drop(&last_stats)) {
                        printf("\n\t[Expirator] Dropping packets!!!\n\n");
                }
#endif

                // Expiration check. 
                pthread_mutex_lock(&epoch_list_ht_lock);

                printf("\t[Expirator] It's time to do expiration check for Epoch %ld.\n", next_epoch_id);
                ip_list = (list_t**) ht_get(&epoch_list_ht, &next_epoch_id, sizeof(long), &value_size);
                if(ip_list != NULL) {
                        list_iterator_t *it = list_iterator_new(*ip_list, LIST_HEAD);
                        list_node_t *next = list_iterator_next(it);
                        while(next != NULL) {
                                printf("\n\t[Expirator] Ip %s expired at Epoch %ld !!!\n\n", \
                                        (char*) next->val, next_epoch_id);
                                list_remove(*ip_list, next);
                                next = list_iterator_next(it);
                        }

                        // Delete iterator, list and hashtable kv entry.
                        list_iterator_destroy(it);
                        list_destroy(*ip_list);
                        ht_remove(&epoch_list_ht, &next_epoch_id, sizeof(long));
                }

                pthread_mutex_unlock(&epoch_list_ht_lock);
                
                next_epoch_id ++;
        }
        return NULL;
}

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
	      struct nfq_data *nfa, void *data) {
	u_int32_t id = process_hb(nfa);
	return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

int main(int argc, char **argv) {
	int fd;
	int rv;
	char buf[2048] __attribute__ ((aligned));
        
        if (signal(SIGINT, signal_handler) == SIG_ERR)
                printf("Can't catch SIGINT\n");

        if(argc != 2) {
                printf("./hb_queue [timeout ms]\n");
                return -1;
        } else 
                expiration_interval = atoi(argv[1]);
        
        ht_init(&epoch_list_ht, HT_NONE, 0.05);

#ifdef S2S
        struct sockaddr_in self_server;
        
        ht_init(&epoch_sem_ht, HT_NONE, 0.05);
        
        self_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&self_server, '0', sizeof(self_server));
        self_server.sin_family = AF_INET;
        self_server.sin_addr.s_addr = inet_addr(SELF_IP);
        self_server.sin_port = htons(SELF_PORT);

        if(connect(self_sockfd, (struct sockaddr *) &self_server, sizeof(self_server)) < 0) {
                fprintf(stderr, "Error: Failed to connect to self server.\n");
                return -1;
        } else {
                printf("[hb_queue] Local send-self server connected\n");
        }
#endif
    
        running = 1;
        pthread_create(&expirator_tid, NULL, expirator, NULL);

	h = nfq_open();
	if(!h) {
	        fprintf(stderr, "error during nfq_open()\n");
                exit(1);
	}

	if(nfq_unbind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_unbind_pf()\n");
		exit(1);
	}

	if(nfq_bind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_bind_pf()\n");
		exit(1);
	}

	qh = nfq_create_queue(h, 0, &cb, NULL);
	if(!qh) {
		fprintf(stderr, "error during nfq_create_queue()\n");
		exit(1);
	}

	if(nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
		fprintf(stderr, "can't set packet_copy mode\n");
		exit(1);
	}

	fd = nfq_fd(h);

	while(running) {
                if((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) {
            		printf("pkt received\n");
			nfq_handle_packet(h, buf, rv);
			continue;
		}
		/* if your application is too slow to digest the packets that
		 * are sent from kernel-space, the socket buffer that we use
		 * to enqueue packets may fill up returning ENOBUFS. Depending
		 * on your application, this error may be ignored. Please, see
		 * the doxygen documentation of this library on how to improve
		 * this situation.
		 */
		if(rv < 0 && errno == ENOBUFS) {
			printf("losing packets!\n");
			continue;
		}
		perror("recv failed");
		break;
	}
       
        pthread_join(expirator_tid, NULL);

        clear_all();
        
        return 0;
}
