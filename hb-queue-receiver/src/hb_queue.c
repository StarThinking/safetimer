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

#include "utility.h"
#include "hashtable.h"
#include "hb_config.h"

static struct nfq_handle *h;
static struct nfq_q_handle *qh;

static hash_table heartbeat_ht, sem_ht;
static pthread_mutex_t heartbeat_ht_lock, sem_ht_lock;
static pthread_mutex_t lock;

static long base_time;
static long expiration_interval;
static pthread_t expirator_tid;
static int self_sockfd;
static int running = 1;

#define ARRAY_SIZE 100
static int received_self_msg_array[ARRAY_SIZE][IRQ_NUM]; 
static int array_round = 1; // recond the round so that clear is not needed for new round

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

static void clear_up(int signo) {
        printf("destroy nfqueue\n");
        destroy_nfqueue();
        
        printf("destory heartbeat_ht\n");
        ht_destroy(&heartbeat_ht);
        
        printf("destory sem_ht\n");
        ht_destroy(&sem_ht);
        
        printf("close self_sockfd\n");
        close(self_sockfd);
        
        printf("cancel thread expirator_tid\n");
        pthread_cancel(expirator_tid);
        
        exit(0);
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
                
                printf("[S2S] All S2S messages for epoch %ld are received.\n", epoch_id);

                // reach the end of a round
                if(array_index == (ARRAY_SIZE -1))
                        array_round ++;    
                
                pthread_mutex_lock(&sem_ht_lock); 
                sem_t **sem = (sem_t**) ht_get(&sem_ht, &epoch_id, sizeof(long), &value_size);
                pthread_mutex_unlock(&sem_ht_lock);
                
                if(sem != NULL) {
                        sem_post(*sem);
                        printf("[S2S] The semaphore for epoch %ld is posted.\n", epoch_id);
                } else 
                        printf("[S2S] Error: The semaphore for epoch %ld not found!\n", epoch_id);
        }

        /*
         * Packets sent not from SELF_IP are heartbeat messages.
         * The content of this UDP packet, beginning at the 28th byte, consists of one 8-byte long.
         * The long contains send_time. 
         */
        if(strcmp(saddr, SELF_IP) != 0) {
                long *content = (long *)(data + 28);
                long send_time = *content;
                long epoch_id = round_to_epoch(send_time); // get the epoch id by sending time

                printf("[Receiver] Heartbeat message from %s for epoch (%ld, %ld) sent at %ld received at time %ld.\n", saddr, epoch_id, round_to_interval(epoch_id), send_time, now());
                
                if(epoch_id < 0)
                        goto ret;

                pthread_mutex_lock(&heartbeat_ht_lock); 
                
                // Remove the key-value entry of epoch_id
                if(ht_contains(&heartbeat_ht, &epoch_id, sizeof(long))) {
                        ht_remove(&heartbeat_ht, &epoch_id, sizeof(long));
                        printf("[Receiver] Epoch %ld removed.\n", epoch_id);
                } else {
                        printf("[Receiver] Waring: Epoch %ld not found when removing!\n", epoch_id);
                }
               
                // Add next epoch
                epoch_id ++;
                ht_insert(&heartbeat_ht, &epoch_id, sizeof(long), &epoch_id, sizeof(long)); // the value does not matters
               
                // Check whether addition is successfull
                if(ht_contains(&heartbeat_ht, &epoch_id, sizeof(long)))
                        printf("[Receiver] Added epoch %ld as the next epoch.\n", epoch_id);
                else
                        printf("[Receiver] Waring: Filed to add epoch %ld as the next timeout epoch.\n", epoch_id);

                pthread_mutex_unlock(&heartbeat_ht_lock); 
        } 

ret:
        return id;
}

/*
 * Thread for expiration check
 */
void *expirator(void *arg) {
        long next_epoch_id;

        base_time = now();
        next_epoch_id = round_to_epoch(now()); 
        printf("[Expirator] Thread started with base_time = %ld, expiration_interval = %ld, next_epoch_id = %ld\n", base_time, expiration_interval, next_epoch_id);
        
        while(1) {
                long current_time = now();
                long next_expiration_time = round_to_interval(next_epoch_id);

                if(next_expiration_time > current_time) {
                        struct timespec sleep_ts = sleep_time(next_expiration_time - current_time);
                        nanosleep(&sleep_ts, NULL);
                        continue;
                }             

                // Allocate and initialize the memory of a new semaphore
                sem_t *sem_p = (sem_t*) calloc(1, sizeof(sem_t));
                sem_init(sem_p, 0, 0);

                pthread_mutex_lock(&sem_ht_lock); 
                ht_insert(&sem_ht, &next_epoch_id, sizeof(long), &sem_p, sizeof(sem_t*));
                pthread_mutex_unlock(&sem_ht_lock); 
                
                printf("\t[S2S] Semaphore for epoch %ld inserted.\n", next_epoch_id);
                
                // Send a TCP message containing next_epoch_id to Self Sender.
                if((send(self_sockfd, &next_epoch_id, MSGSIZE, 0)) != MSGSIZE) {
                        fprintf(stderr, "Error: Send to self wrong!\n");
                        goto error;
                }
                
                printf("\t[S2S] Self-message for epoch %ld sent at time %ld.\n", next_epoch_id, now());
                
                sem_wait(sem_p); 

                // Waken up and proceed
                pthread_mutex_lock(&sem_ht_lock); 
                ht_remove(&sem_ht, &next_epoch_id, sizeof(long));
                free(sem_p);
                pthread_mutex_unlock(&sem_ht_lock); 
                
                printf("\t[S2S] Semaphore for epoch %ld removed.\n", next_epoch_id);
                
                pthread_mutex_lock(&heartbeat_ht_lock);
                if(!ht_contains(&heartbeat_ht, &next_epoch_id, sizeof(long))) {
                        printf("\t[Expirator] Not timeout at epoch %ld.\n", next_epoch_id);
                } else { 
                        printf("\n\t[Expirator] Timeouted at epoch %ld!!!\n\n", next_epoch_id);
                        goto timeout;
                }
                pthread_mutex_unlock(&heartbeat_ht_lock);
                
                next_epoch_id ++;
        }
timeout:
        pthread_mutex_unlock(&heartbeat_ht_lock);
error:
        pthread_mutex_lock(&lock); 
//        printf("running = 0\n");
  //      running = 0;
        pthread_mutex_unlock(&lock); 
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
        struct sockaddr_in self_server;
        
        signal(SIGINT, clear_up);
        
        if(argc != 2) {
                printf("./hb_queue [timeout ms]\n");
                return -1;
        } else 
                expiration_interval = atoi(argv[1]);
        
        ht_init(&heartbeat_ht, HT_NONE, 0.05);
        ht_init(&sem_ht, HT_NONE, 0.05);
        
        self_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&self_server, '0', sizeof(self_server));
        self_server.sin_family = AF_INET;
        self_server.sin_addr.s_addr = inet_addr(SELF_IP);
        self_server.sin_port = htons(SELF_PORT);

        if(connect(self_sockfd, (struct sockaddr *) &self_server, sizeof(self_server)) < 0) {
                fprintf(stderr, "Error: Failed to connect to self server.\n");
                return -1;
        } else {
                printf("[hb_queue] local send-self server connected\n");
        }
        
        pthread_create(&expirator_tid, NULL, expirator, NULL);

	printf("opening library handle\n");
	h = nfq_open();
	if(!h) {
	        fprintf(stderr, "error during nfq_open()\n");
                exit(1);
	}

	printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
	if(nfq_unbind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_unbind_pf()\n");
		exit(1);
	}

	printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
	if(nfq_bind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_bind_pf()\n");
		exit(1);
	}

	printf("binding this socket to queue '0'\n");
	qh = nfq_create_queue(h, 0, &cb, NULL);
	if(!qh) {
		fprintf(stderr, "error during nfq_create_queue()\n");
		exit(1);
	}

	printf("setting copy_packet mode\n");
	if(nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
		fprintf(stderr, "can't set packet_copy mode\n");
		exit(1);
	}

	fd = nfq_fd(h);

	while(1) {
                pthread_mutex_lock(&lock); 
                if(running != 1)
                        break;
                pthread_mutex_unlock(&lock); 
		
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

        clear_up(0);
        
        return 0;
}
