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
                put_state(app_id, saddr, now_time());
 
        }
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
