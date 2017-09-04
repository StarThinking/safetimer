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

#include "hashtable.h"
#include "list.h"

extern sem_t init_done_sem;

static int runnable = 1;

static hash_table epoch_list_ht;
static pthread_mutex_t epoch_list_ht_lock;

static struct nfq_handle *h;
static struct nfq_q_handle *qh;
static int fd;

static void cleanup_queue();
//static void cleanup_expire(void *arg);

static pthread_t queue_tid;
static void *queue_recv_loop(void *arg);

static pthread_t expire_checker_tid;
static void *expire_checker(void *arg);

static u_int32_t process_packet(struct nfq_data *tb);
static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, 
        struct nfq_data *nfa, void *data);
static int ip_equal(void *a, void *b);
static void free_val(void *val);

int init_queue() {
        int ret = 0;
        struct timeval read_timeout;
        pthread_attr_t attr;

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

        read_timeout.tv_sec = 1;
        read_timeout.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
        
        ht_init(&epoch_list_ht, HT_NONE, 0.05);

        /* Start thread for queue receiving. */
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&queue_tid, &attr, queue_recv_loop, NULL);
        printf("Queue: receive loop thread started.\n");

        /* Start expiration checker thread. */
        pthread_create(&expire_checker_tid, &attr, expire_checker, NULL);
        printf("Queue: expire checker thread started.\n");
       
        printf("Queue has been initialized successfully.\n");

error:
        return ret;
}

void stop_queue() {
        runnable = 0;
}

static void cleanup_queue() {
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
        
        payload_len = nfq_get_payload(tb, &nf_packet);
        if (payload_len >= 0) {
                iph = (struct iphdr *) nf_packet;
                iphdr_size = iph->ihl << 2;
                inet_ntop(AF_INET, &(iph->saddr), saddr, INET_ADDRSTRLEN);
        }      

        if (iph->protocol == IPPROTO_UDP && strcmp(saddr, BARRIER_SERVER_ADDR) != 0) {
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
                        printf("[Receiver] Error: UDP payload size less than %ld!\n", MSGSIZE*2);
                }

                if (flag == 0) {
                        printf("Queue: request [flag=%ld, epoch=%ld] from %s.\n", flag, epoch, saddr);
                        goto ret;
                } 

                printf("Queue: heartbeat [flag=%ld, epoch=%ld] from %s.\n", flag, epoch, saddr);
                
                if (epoch < 0) {
                        fprintf(stderr, "Queue error: epoch < 0.\n");
                        goto ret;
                }

                pthread_mutex_lock(&epoch_list_ht_lock);

                // Touch the node for the epoch
                ip_list = (list_t**) ht_get(&epoch_list_ht, &epoch, sizeof(long), &value_size);
                if(ip_list != NULL) {
                        list_node_t *ip_to_remove = list_find(*ip_list, saddr);
                        if(ip_to_remove != NULL) {
                                list_remove(*ip_list, ip_to_remove);
                                printf("Queue: removed IP %s for epoch list %ld.\n", saddr, epoch);
                        }
                }

                // Insert Ip into the Ip list of the next epoch
                epoch ++;

                ip_list = (list_t**) ht_get(&epoch_list_ht, &epoch, sizeof(long), &value_size);
                if(ip_list == NULL) {
                        list_t *tmp = list_new();
                        tmp->match = ip_equal;
                        tmp->free = free_val;
                        ip_list = &tmp;
                        ht_insert(&epoch_list_ht, &epoch, sizeof(long), ip_list, sizeof(list_t*));
                        printf("Queue: created and inserted the Ip list for epoch %ld.\n", epoch);
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
        //printf("process_packet id = %d.\n", id);
        return id;
}

/*
 * Thread for expiration check
 */
static void *expire_checker(void *arg) {
        list_t **ip_list;
        size_t value_size;
        long timeout;
        long epoch;
        long diff_time;
        
        sem_wait(&init_done_sem);
        printf("Queue: wait is done for expire thread.\n");

        while (runnable) {
                epoch = time_to_epoch(now_time());
                timeout = epoch_to_time(epoch);

                if ((diff_time = timeout - now_time()) > 0) {
                        struct timespec ts = time_to_timespec(diff_time);
                        nanosleep(&ts, NULL);
                }

                // Expiration check. 
                pthread_mutex_lock(&epoch_list_ht_lock);

                if(!runnable)
                        break;

                printf("good to check epoch %ld.\n", epoch);
                ip_list = (list_t**) ht_get(&epoch_list_ht, &epoch, sizeof(long), &value_size);
                if (ip_list != NULL) {
                        list_iterator_t *it = list_iterator_new(*ip_list, LIST_HEAD);
                        list_node_t *next = list_iterator_next(it);
                        while (next != NULL) {  
                                printf("\tQueue: node %s timeout for epoch %ld !\n", 
                                        (char*) next->val, epoch);
                                
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
        
        //pthread_cleanup_pop(1);
        
        return NULL;
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

        while (runnable) {
                if ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) { 
                        nfq_handle_packet(h, buf, rv);
                        continue;
                }

                err = errno;

                if (rv < 0 && err == ENOBUFS) {
                        fprintf(stderr, "Queue is dropping packets.\n");
                        continue;
                }

                if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
                        /* non-blocking operation returned EAGAIN or EWOULDBLOCK */
                        continue;
                }

                fprintf(stderr, "Queue: this point should not be touched.\n");
                
                break;
        }
        
        cleanup_queue();

        pthread_exit(NULL);

        return NULL;
}

static int ip_equal(void *a, void *b) {
        return 0 == strcmp((char*)a, (char*)b);
}

static void free_val(void *val) {
        free(val);
}

