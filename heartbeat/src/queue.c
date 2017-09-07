#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <linux/types.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/netfilter.h> 
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <semaphore.h>

#include "hb_config.h"
#include "queue.h"
#include "helper.h"
#include "barrier.h" // for send_barrier_message()
#include "drop.h"

#include "hashtable.h"
#include "list.h"

#ifdef CONFIG_BARRIER

static sem_t barrier_all_processed;

#define ARRAY_SIZE 100
/* Recond the round so that clear is not needed for new round. */
static int array_round = 1;
static int received_barrier_msg_array[ARRAY_SIZE][IRQ_NUM];

#endif

#ifdef CONFIG_DROP

static long waive_check_epoch;

#endif

extern sem_t init_done;
static int init_done_flag = 0;

static hash_table epoch_list_ht;
static pthread_mutex_t epoch_list_ht_lock;

static struct nfq_handle *h;
static struct nfq_q_handle *qh;
static int fd;

static pthread_t queue_tid;
static void *queue_recv_loop(void *arg);
static void cleanup(void *arg);

static pthread_t expire_checker_tid;
static void *expire_checker(void *arg);
static void expiration_check_for_epoch(long epoch);

static u_int32_t process_packet(struct nfq_data *tb);
static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, 
        struct nfq_data *nfa, void *data);
static int ip_equal(void *a, void *b);
static void free_val(void *val);

cb_t timeout_cb;

int init_queue(cb_t callback) {
        int ret = 0;
        struct timeval read_timeout;

        h = nfq_open();
        if (!h) {
                fprintf(stderr, "Queue: error during nfq_open().\n");
                goto error;
        }

        if (nfq_unbind_pf(h, AF_INET) < 0) {
                fprintf(stderr, "Queue: error during nfq_unbind_pf().\n");
                goto error;
        }

        qh = nfq_create_queue(h, 0, &cb, NULL);
        if (!qh) {
                fprintf(stderr, "Queue: error during nfq_create_queue().\n");
                goto error;
        }
        
        if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
                fprintf(stderr, "Queue: can't set packet_copy mode.\n");
                goto error;
        }

        /* Save queue fd. */
        fd = nfq_fd(h);

        /* Set queue recv to be on-blocking. */
        read_timeout.tv_sec = 1;
        read_timeout.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
        
        ht_init(&epoch_list_ht, HT_NONE, 0.05);

#ifdef CONFIG_BARRIER
        sem_init(&barrier_all_processed, 0, 0);
#endif

        /* Start thread for queue receiving. */
        pthread_create(&queue_tid, NULL, queue_recv_loop, NULL);
        printf("Queue: receive loop thread started.\n");

        /* Start expiration checker thread. */
        pthread_create(&expire_checker_tid, NULL, expire_checker, NULL);
        timeout_cb = callback;
        printf("Queue: expire checker thread started.\n");
       
        printf("Queue has been initialized successfully.\n");

error:
        return ret;
}

void cancel_queue() {
	pthread_cancel(expire_checker_tid);
	pthread_cancel(queue_tid);
}

void join_queue() {
	pthread_join(expire_checker_tid, NULL);
	printf("Queue expire_checker thread joined.\n");
	
	pthread_join(queue_tid, NULL);
	printf("Queue queue recv thread joined.\n");
}

static void cleanup(void *arg) {
        printf("Clean up queue.\n");

        nfq_destroy_queue(qh);

#ifdef INSANE
        /* normally, applications SHOULD NOT issue this command, since
         * it detaches other programs/sockets from AF_INET, too ! */
        nfq_unbind_pf(h, AF_INET);
#endif
        nfq_close(h);
        
        ht_destroy(&epoch_list_ht);
}

static u_int32_t process_packet(struct nfq_data *tb) {
	int id = 0;
        struct nfqnl_msg_packet_hdr *ph;
        int payload_len = 0;
        unsigned char *nf_packet;
        struct iphdr* iph;
        unsigned short iphdr_size;
        char saddr[INET_ADDRSTRLEN];

        ph = nfq_get_msg_packet_hdr(tb);
        if (ph)
                id = ntohl(ph->packet_id);

        /* Initialization is NOT done. */
        if(init_done_flag == 0) 
                goto ret;

        payload_len = nfq_get_payload(tb, &nf_packet);
        if (payload_len >= 0) {
                iph = (struct iphdr *) nf_packet;
                iphdr_size = iph->ihl << 2;
                inet_ntop(AF_INET, &(iph->saddr), saddr, INET_ADDRSTRLEN);
        }      

        if (iph->protocol == IPPROTO_UDP) {
        //if (iph->protocol == IPPROTO_UDP && strcmp(saddr, BARRIER_SERVER_ADDR) != 0) {
                unsigned short udphdr_size = 8;
                unsigned short offset = iphdr_size + udphdr_size;
                long flag = -1;
                long epoch = -1;
                size_t value_size;
                list_t **ip_list;

                //printf("payload_len = %d, offset = %d\n", payload_len, offset);
                // payload size check
                if (payload_len >= offset + MSGSIZE*2) {
                        flag = *((long*)(nf_packet + offset));
                        epoch = *((long *)(nf_packet + offset + MSGSIZE));
                } else {
                        printf("Queue: UDP payload size less than %ld!\n", MSGSIZE*2);
                        goto ret;
                }
                
                /* Skip furthur process for requests. */
                if (flag == 0) {
                        printf("Queue: request [flag=%ld, epoch=%ld] from node %s.\n", flag, epoch, saddr);
                        goto ret;
                } 

                printf("Queue: heartbeat for epoch %ld from node %s.\n",  epoch, saddr);
                
                if (epoch < 0) {
                        fprintf(stderr, "Queue error: epoch < 0.\n");
                        goto ret;
                }

                pthread_mutex_lock(&epoch_list_ht_lock);

                // Touch the node for the epoch
                ip_list = (list_t**) ht_get(&epoch_list_ht, &epoch, sizeof(long), &value_size);
                if (ip_list != NULL) {
                        list_node_t *ip_to_remove = list_find(*ip_list, saddr);
                        if (ip_to_remove != NULL) {
                                list_remove(*ip_list, ip_to_remove);
                                printf("Queue: remove node %s from epoch list %ld.\n", saddr, epoch);
                        }
                }

                // Insert Ip into the Ip list of the next epoch
                epoch ++;

                ip_list = (list_t**) ht_get(&epoch_list_ht, &epoch, sizeof(long), &value_size);
                if (ip_list == NULL) {
                        list_t *tmp = list_new();
                        tmp->match = ip_equal;
                        tmp->free = free_val;
                        ip_list = &tmp;
                        ht_insert(&epoch_list_ht, &epoch, sizeof(long), ip_list, sizeof(list_t*));
                        //printf("Queue: create new node list for epoch %ld.\n", epoch);
                } 
	
		if (list_find(*ip_list, saddr) == NULL) {
                        // Allocate the memory of _saddr in heap
                        char *_saddr = (char*) calloc(INET_ADDRSTRLEN, sizeof(char));
                        strcpy(_saddr, saddr);
                        list_rpush(*ip_list, list_node_new(_saddr));
                        printf("Queue: add node %s into the node list for epoch %ld.\n", saddr, epoch);
                }

                pthread_mutex_unlock(&epoch_list_ht_lock);
        }

#ifdef CONFIG_BARRIER
	
        //if (iph->protocol == IPPROTO_TCP && strcmp(saddr, BARRIER_CLIENT_ADDR) == 0) {
        if (iph->protocol == IPPROTO_TCP) {
		int i, array_index;
                struct tcphdr *tcp = ((struct tcphdr *) (nf_packet + iphdr_size));
                unsigned short tcphdr_size = (tcp->doff << 2);
                unsigned short offset = iphdr_size + tcphdr_size;
                long epoch = -1;
                long queue_index = -1;

                // payload size check
                if (payload_len >= offset + MSGSIZE*2) {
                        epoch = *((long *)(nf_packet + offset));
                        queue_index = *((long *)(nf_packet + offset + MSGSIZE));
                        
                        if(epoch <= 0 || queue_index < 0) {
                                fprintf(stderr, "Queue: epoch or queue_index values wrong.\n");
                                goto ret;
                        }
                        
                        //printf("Queue: epoch = %ld, queue_index = %ld\n", epoch, queue_index);
                } else {
                        printf("Queue: error happens as tcp payload size less than %ld!\n", MSGSIZE*2);
                }

                // get array index; if it's a new round, increase round
                array_index = epoch % ARRAY_SIZE;

                if (received_barrier_msg_array[array_index][queue_index] != array_round) {
                        received_barrier_msg_array[array_index][queue_index] = array_round;
                } else { // the S2S message for the ring is redundant
                        goto ret;
                }

                for (i=0; i<IRQ_NUM; i++) {
                        if(received_barrier_msg_array[array_index][i] != array_round)
                                goto ret;
                }

                printf("Queue: all barrier messages for epoch %ld are received.\n", epoch);

                // reach the end of a round
                if(array_index == (ARRAY_SIZE -1))
                        array_round ++;

                sem_post(&barrier_all_processed);
        }

#endif

ret:
        //printf("process_packet id = %d.\n", id);
        return id;
}

/*
 * Thread for expiration check
 */
static void *expire_checker(void *arg) {
        long timeout;
        long epoch_to_check, epoch_now;
        long diff_time;
        
        sem_wait(&init_done);
        /* So that queue recv thread will perform packet processing. */
        init_done_flag = 1;
        printf("Checker: initialization is done and start to expiration check loop.\n");

        while (1) {
                epoch_to_check = time_to_epoch(now_time());
                timeout = epoch_to_time(epoch_to_check);

                if ((diff_time = timeout - now_time()) > 0) {
                        struct timespec ts = time_to_timespec(diff_time);
                        nanosleep(&ts, NULL);
                }
               
                do {
                        expiration_check_for_epoch(epoch_to_check);
                        epoch_now = time_to_epoch(now_time());

                        if (epoch_to_check + 1 < epoch_now)
                                printf("Checker: more than 1 epoch should be checked!\n");

                        epoch_to_check ++;
                } while (epoch_to_check < epoch_now);

        }
        
        return NULL;
}

static void expiration_check_for_epoch(long epoch) {
        list_t **ip_list;
        size_t value_size;
        int ret;

#ifdef CONFIG_BARRIER
                
        send_barrier_message(epoch);
        printf("\tChecker: barrier messages for epoch %ld are sent, waiting for sem post.\n", epoch);
        sem_wait(&barrier_all_processed);
        printf("\tChecker: all barrier messages for epoch %ld are processed.\n", epoch);

#endif

#ifdef CONFIG_DROP

        if ((ret = if_drop_happened()) > 0) {
                if (ret / NICDROP) {
                        ret = ret % NICDROP;
                        printf("\tChecker: drop happens in NIC.");
                }

                if (ret / DEVDROP) {
                        ret = ret % DEVDROP;
                        printf("\tChecker: drop happens in driver or kernel_dev.");
                }
                
                if (ret / QUEUEDROP) {
                        printf("\tChecker: drop happens in NFQueue.");
                }

                waive_check_epoch = time_to_epoch(now_time());
                
                printf(" Waive expire check for epoch <= %ld.\n", waive_check_epoch);
        } else {
                printf("\tChecker: no drop.\n");
        }

#endif

        printf("\tChecker: good to check expiration for epoch %ld.\n", epoch);
                
        /* Before perfroming expiration check, grab the lock. */ 
        pthread_mutex_lock(&epoch_list_ht_lock);
                
        ip_list = (list_t**) ht_get(&epoch_list_ht, &epoch, sizeof(long), &value_size);
        if (ip_list != NULL) {
               list_iterator_t *it = list_iterator_new(*ip_list, LIST_HEAD);
               list_node_t *next = list_iterator_next(it);
               while (next != NULL) {  
                        if (epoch <= waive_check_epoch) {
                                printf("\n\tChecker: although node %s timeouts for epoch %ld "
                                        "but it's waived due to packet drop.\n\n", 
                                        (char*) next->val, epoch);
                        } else {
                                printf("\n\tChecker: node %s timeout for epoch %ld !\n\n", 
                                        (char*) next->val, epoch);
                                
                                /* Invoke application-defined callback function. */
                                timeout_cb();
                        }
                                
                        list_remove(*ip_list, next);
                        next = list_iterator_next(it);
               }

               // Delete iterator, list and hashtable kv entry.
               list_iterator_destroy(it);
               list_destroy(*ip_list);
               ht_remove(&epoch_list_ht, &epoch, sizeof(long));
        } 
        
        pthread_mutex_unlock(&epoch_list_ht_lock);
}

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
        struct nfq_data *nfa, void *data) {
        u_int32_t id = process_packet(nfa);
        return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

static void *queue_recv_loop(void *arg) {
        int rv;
        char buf[2048] __attribute__ ((aligned));
        int err;

	pthread_cleanup_push(cleanup, NULL);

        while (1) {
                if ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) { 
                        nfq_handle_packet(h, buf, rv);
                        continue;
                }

                err = errno;

                if (rv < 0 && err == ENOBUFS) {
                        queue_dropped_pkt_current ++;
                        fprintf(stderr, "Queue is dropping packets.\n");
                        continue;
                }

                if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
                        // non-blocking operation returned EAGAIN or EWOULDBLOCK
                        continue;
                }

                fprintf(stderr, "Queue: this point should not be touched.\n");
                
                break;
        }
        
	pthread_cleanup_pop(1);

        return NULL;
}

static int ip_equal(void *a, void *b) {
        return 0 == strcmp((char*)a, (char*)b);
}

static void free_val(void *val) {
        free(val);
}

