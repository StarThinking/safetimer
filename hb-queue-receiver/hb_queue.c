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

#define PORT 6001
#define LOCALIP "10.0.0.12"
#define MSGSIZE sizeof(long)

static pthread_t expirator_tid;
static int sockfn;

void sig_handler(int signo) {
        if(signo == SIGINT) {
                close(sockfn);
                pthread_cancel(expirator_tid);
        }
        exit(0);
}

/* process packet and returns packet id */
static u_int32_t process_pkt (struct nfq_data *tb) {
        int id = 0;
        struct nfqnl_msg_packet_hdr *ph;
	int payload_len;
	unsigned char *data;
        
        ph = nfq_get_msg_packet_hdr(tb);
        if(ph)
                id = ntohl(ph->packet_id);
       
        payload_len = nfq_get_payload(tb, &data);

        if(payload_len >= 0) {
                struct iphdr* iph;
                char saddr[INET_ADDRSTRLEN];
                char daddr[INET_ADDRSTRLEN];
                uint16_t sport = 0, dport = 0;
                
                iph = (struct iphdr *) data;
                printf("size of iphdr = %lu\n", sizeof(struct iphdr));
                inet_ntop(AF_INET, &(iph->saddr), saddr, INET_ADDRSTRLEN);
                inet_ntop(AF_INET, &(iph->daddr), daddr, INET_ADDRSTRLEN);
        
                if(iph->protocol == IPPROTO_TCP) {
                        struct tcphdr *tcp;
                        tcp = ((struct tcphdr *) (data + (iph->ihl << 2)));
                        sport = ntohs(tcp->source);
                        dport = ntohs(tcp->dest);
                }   
                
                if(strcmp(saddr, LOCALIP) == 0) {
                        printf("[hb_queue] local hb received.\n");
                } else {
                        printf("%s : %u --> %s : %u\n", saddr, sport, daddr, dport);
                }
                
                long *now = (long *)(data + 52);
                printf("now = %ld\n", *now);
        }

	return id;
}
	
static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
	      struct nfq_data *nfa, void *data) {
	u_int32_t id = process_pkt(nfa);
	return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

// thread for checking expiration
void *expirator(void *arg) {
        int sockfn = *(int *) arg;
        int ret;
        long buf = 1;

        while(1) {
                ret = send(sockfn, &buf, MSGSIZE, 0);
                sleep(1);
        }
}

int main(int argc, char **argv) {

        signal(SIGINT, sig_handler);
        
        struct nfq_handle *h;
	struct nfq_q_handle *qh;
	//struct nfnl_handle *nh;
	int fd;
	int rv;
	char buf[2048] __attribute__ ((aligned));

        struct sockaddr_in serv_addr;
        sockfn = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(LOCALIP);
        serv_addr.sin_port = htons(PORT);

        if(connect(sockfn, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                fprintf(stderr, "Error: connect failed.\n");
                return -1;
        } else {
                printf("[hb_queue local send-self server connected\n");
                pthread_create(&expirator_tid, NULL, expirator, &sockfn);
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
