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

#include "hb_common.h"
#include "queue.h"
#include "helper.h"
#include "state_server.h" // for node_state

#include "hashtable.h"
#include "list.h"

#ifdef CONFIG_BARRIER
static sem_t barrier_all_processed;
static int barrier_flush(long epoch);
static long epoch_no_sem_post;

#define ARRAY_SIZE 100
/* Recond the round so that clear is not needed for new round. */
static int array_round = 1;
static int received_barrier_msg_array[ARRAY_SIZE][IRQ_NUM];
#endif



static struct nfq_handle *h;
static struct nfq_q_handle *qh;
static int fd;

static pthread_t queue_tid;
static void *queue_recv_loop(void *arg);
static void cleanup(void *arg);

static u_int32_t process_packet(struct nfq_data *tb);
static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, 
        struct nfq_data *nfa, void *data);

int init_queue() {
        int ret = 0;
        struct timeval read_timeout;

#ifdef CONFIG_BARRIER
        sem_init(&barrier_all_processed, 0, 0);
#endif

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

        /* Set queue recv to be non-blocking. */
        read_timeout.tv_sec = 1;
        read_timeout.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
        
        /* Start thread for queue receiving. */
        pthread_create(&queue_tid, NULL, queue_recv_loop, NULL);
        printf("Queue: receive loop thread started.\n");

error:
        return ret;
}

void cancel_queue() {
	pthread_cancel(queue_tid);
}

void join_queue() {
	pthread_join(queue_tid, NULL);
	printf("Queue queue recv thread joined.\n");
}

static void cleanup(void *arg) {
        printf("Clean up queue.\n");

#ifdef INSANE
        /* normally, applications SHOULD NOT issue this command, since
         * it detaches other programs/sockets from AF_INET, too ! */
        nfq_unbind_pf(h, AF_INET);
#endif
        nfq_close(h);
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

        if (iph->protocol == IPPROTO_UDP) {
                unsigned short udphdr_size = 8;
                unsigned short offset = iphdr_size + udphdr_size;
                long message;
                long app_id;

                // payload size check
                if (payload_len >= offset + MSGSIZE) {
                        message = *((long*)(nf_packet + offset));
                } else {
                        printf("Queue: UDP payload size less than %ld!\n", MSGSIZE);
                        goto ret;
                }
                
                printf("Queue: heartbeat with message %ld from node %s.\n", message, saddr);
                app_id = message;
                receive_heartbeat(app_id, saddr, now_time());
 
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
                        
                        printf("Queue: epoch = %ld, queue_index = %ld\n", epoch, queue_index);
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
                
                // ignore outdated barriers
                if (epoch_no_sem_post && epoch <= epoch_no_sem_post) 
                        printf("no sem post for barriers for epoch <= %ld.\n", epoch_no_sem_post);
                else
                        sem_post(&barrier_all_processed);
         }
#endif

ret:
        return id;
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
                        //queue_dropped_pkt_current ++;
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

/*
#ifdef CONFIG_BARRIER
static int barrier_flush(long epoch) {
        struct timespec wait_timeout;
        int s;

        wait_timeout = time_to_timespec(epoch_to_time(time_to_epoch(now_time())));
        send_barrier_message(epoch);
        printf("\tChecker: barrier messages for epoch %ld are sent, waiting for sem post.\n", epoch);
        while ((s = sem_timedwait(&barrier_all_processed, &wait_timeout)) == -1 && errno == EINTR) 
                continue; // Restart if interrupted by handler 

        if (s == -1) {
                if (errno == ETIMEDOUT)
                        printf("sem_timedwait() timed out\n");
                else
                        perror("sem_timedwait");
                epoch_no_sem_post = epoch;
                printf("set epoch_no_sem_post as %ld\n", epoch_no_sem_post);
                return 1;
        } else {
                printf("sem_timedwait() succeeded\n");
        }
        printf("\tChecker: all barrier messages for epoch %ld are processed.\n", epoch);
        return 0;
}
#endif
*/
