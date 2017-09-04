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

static int runnable = 1;

static struct nfq_handle *h;
static struct nfq_q_handle *qh;
static int fd;

static void cleanup_queue(void *arg);

static pthread_t queue_tid;
static void *queue_recv_loop(void *arg);

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
        
        read_timeout.tv_sec = 1;
        read_timeout.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
        
        /* Start thread for queue receiving. */
        pthread_create(&queue_tid, NULL, queue_recv_loop, NULL);
        printf("Queue: receive loop thread started.\n");
       
        printf("Queue has been initialized successfully.\n");

error:
        return ret;
}

static void cleanup_queue(void *arg) {
        printf("Clean up queue.\n");

        nfq_destroy_queue(qh);

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

ret:
        printf("process_packet id = %d.\n", id);
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
        
        while (runnable) {
               // sleep(1);
                if ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) { 
                        nfq_handle_packet(h, buf, rv);
                        continue;
                }
                err = errno;

                if (rv < 0 && errno == ENOBUFS) {
                        fprintf(stderr, "Queue is dropping packets.\n");
                        continue;
                }

                if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
                        printf("non-blocking operation returned EAGAIN or EWOULDBLOCK\n");
                        continue;
                }

                fprintf(stderr, "Queue: this point should not be touched.\n");
                
                break;
        }

        cleanup_queue(NULL);

        return NULL;
}

int main(int argc, char *argv[]) {
        init_queue();

        sleep(10);

        runnable = 0;

        pthread_join(queue_tid, NULL);

        printf("joined\n");

}
