#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>		/* for NF_ACCEPT */
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/ipv6.h>
#include <signal.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "./hashset.c/hashset.h"
#include "./hashset.c/hashset_itr.h"

#define PORT 6001
#define LOCALIP "10.0.0.12"
#define MSGSIZE sizeof(long)
#define SELFMSG 1110

static hashset_t set; 
static long expiration_interval;
static long next_expiration_time;
static pthread_mutex_t lock;
static sem_t self_msg_received;

static pthread_t expirator_tid;
static int selffd;

void sig_handler(int signo) {
        if(signo == SIGINT) {
                close(selffd);
                pthread_cancel(expirator_tid);
        }
        exit(0);
}

long now() {
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        return spec.tv_sec * 1000 + spec.tv_nsec/1.0e6;
}

long round_to_interval(long time) {
        return (time / expiration_interval + 1) * expiration_interval;
}

/* process packet and returns packet id */
static u_int32_t process_pkt(struct nfq_data *tb) {
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
                //printf("size of iphdr = %lu\n", sizeof(struct iphdr));
                inet_ntop(AF_INET, &(iph->saddr), saddr, INET_ADDRSTRLEN);
        }
              
        if(strcmp(saddr, LOCALIP) == 0) {
                printf("Received send to self message.\n");
                sem_post(&self_msg_received);
        } else {
                long *timestamp = (long *)(data + 52);
                printf("Receievd heartbeat from %s, timestamp = %ld\n", saddr, *timestamp);
                        
                long tick_time = round_to_interval(*timestamp); // find the current expire time
                long expire_time = round_to_interval(now() + expiration_interval); // future expire time
                        
                if(tick_time >= expire_time) { // nothing to do
                       return id;
                }

                pthread_mutex_lock(&lock);
                if(hashset_remove(set, &tick_time) == 0) // item not exist
                        printf("Warning: Cannot remove tick_time %ld! set size is %zu\n", tick_time, hashset_num_items(set));
                long *tmp = (long*) calloc(1, sizeof(long));
                *tmp = expire_time;
                hashset_add(set, tmp);
                printf("Add expire_time = %ld\n", tick_time);
                pthread_mutex_unlock(&lock);
        }        
	return id;
}

struct timespec sleep_time(long sleep_t) {
        struct timespec sleep_ts;
        sleep_ts.tv_sec = sleep_t / 1000;
        sleep_ts.tv_nsec = (sleep_t % 1000) * 1.0e6;
        return sleep_ts;
}

int send_to_self() {
        int ret;
        const long send_self_msg = SELFMSG;
        
        ret = send(selffd, &send_self_msg, MSGSIZE, 0);
        if(ret == MSGSIZE)
                return 0;
        else {
                fprintf(stderr, "Warning: Write ret = %d\n", ret);
                return -1;
        }
}

// thread for checking expiration
void *expirator(void *arg) {
        struct timespec sleep_ts;
        next_expiration_time = round_to_interval(now());
        while(1) {
                long current_time = now();
                if(next_expiration_time > current_time) {
                        sleep_ts = sleep_time(next_expiration_time - now());
                        nanosleep(&sleep_ts, NULL);
                        continue;
                }
                
                if(send_to_self() < 0) {
                        fprintf(stderr, "Error: Send to self wrong!\n");
                        break;
                }                   
                sem_wait(&self_msg_received);
                
                pthread_mutex_lock(&lock);
                if(hashset_is_member(set, &next_expiration_time) != 0) { // exist
                        hashset_remove(set, &next_expiration_time);
                        printf("Timeouted at next_expiration_time %ld !!!\n", next_expiration_time);
                }
                next_expiration_time += expiration_interval; // add one maybe not enough
                pthread_mutex_unlock(&lock);
        }
}

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
	      struct nfq_data *nfa, void *data) {
	u_int32_t id = process_pkt(nfa);
	return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

int main(int argc, char **argv) {
        struct nfq_handle *h;
	struct nfq_q_handle *qh;
	//struct nfnl_handle *nh;
	int fd;
	int rv;
	char buf[2048] __attribute__ ((aligned));
        struct sockaddr_in serv_addr;
        set = hashset_create();

        if(argc != 2) {
                printf("./hb_queue [timeout ms]\n");
                return -1;
        } 
        expiration_interval = atoi(argv[1]);
        
        signal(SIGINT, sig_handler);

        sem_init(&self_msg_received, 0, 0);
        
        selffd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(LOCALIP);
        serv_addr.sin_port = htons(PORT);

        if(connect(selffd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                fprintf(stderr, "Error: connect failed.\n");
                return -1;
        } else {
                pthread_create(&expirator_tid, NULL, expirator, NULL);
                printf("[hb_queue] local send-self server connected\n");
        }

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
	qh = nfq_create_queue(h,  0, &cb, NULL);
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

	for(;;) {
		if((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) {
//			printf("pkt received\n");
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

	exit(0);
}
