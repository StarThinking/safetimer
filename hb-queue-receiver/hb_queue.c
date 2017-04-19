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

#include "./hashtable/src/hashtable.h"

#define PORT 6001
#define SELFIP "10.0.0.12"
#define MSGSIZE sizeof(long)
#define SELFMSG 1110

static hash_table ht, sem_ht;
static long base_time;
static long expiration_interval;
static pthread_mutex_t lock;

static pthread_t expirator_tid;
static int selffd;

void sig_handler(int signo) {
        ht_destroy(&ht);
        ht_destroy(&sem_ht);
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

// return epoch id
long round_to_epoch(long time) {
        time -= base_time;
        
        if(time < 0)
                return -1;

        return (long)(time / expiration_interval + 1);
}

long round_to_interval(long id) {
        return base_time + (id * expiration_interval);
}

struct timespec sleep_time(long sleep_t) {
        struct timespec sleep_ts;
        sleep_ts.tv_sec = sleep_t / 1000;
        sleep_ts.tv_nsec = (sleep_t % 1000) * 1.0e6;
        return sleep_ts;
}

long random_at_most(long max) {
        unsigned long
        // max <= RAND_MAX < ULONG_MAX, so this is okay.
        num_bins = (unsigned long) max + 1,
        num_rand = (unsigned long) RAND_MAX + 1,
        bin_size = num_rand / num_bins,
        defect   = num_rand % num_bins;

        long x;
        do {
                x = random();
        }
        // This is carefully written not to overflow
        while (num_rand - defect <= (unsigned long)x);

        // Truncated division is intentional
        return x/bin_size;
}

int send_to_self(int epoch_id) {
        int ret;
        const long send_self_msg = (long) epoch_id;
        
        ret = send(selffd, &send_self_msg, MSGSIZE, 0);
        if(ret == MSGSIZE)
                return 0;
        else {
                fprintf(stderr, "Warning: Write ret = %d\n", ret);
                return -1;
        }
}

// process packet and returns packet id 
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
                //printf("size of iphdr = %lu\n", sizeof(struct iphdr));
                inet_ntop(AF_INET, &(iph->saddr), saddr, INET_ADDRSTRLEN);
        }
        
        // sleep [0, expiration_interval]
/*        long rand_sleep_time = random_at_most(expiration_interval);
        if(rand_sleep_time > 0.9 * expiration_interval) {
                struct timespec sleep_ts = sleep_time(rand_sleep_time + expiration_interval);
                printf("[Receiver] Sleep %ld ms.\n", rand_sleep_time);
                nanosleep(&sleep_ts, NULL);
        }
  */            
        if(strcmp(saddr, SELFIP) == 0) {
                long *content = (long *)(data + 52);
                long epoch_id  = *content; 
                long ring_id  = *(content + 1); 
                
                printf("[S2S] S2S message for epoch %ld for ring %ld received at time %ld.\n", epoch_id, ring_id, now());

                /*size_t value_size;
                pthread_mutex_lock(&lock); 
                sem_t **sem = (sem_t**)ht_get(&sem_ht, epoch_id, sizeof(long), &value_size);
                pthread_mutex_unlock(&lock);
                
                if(sem == NULL) {
                        printf("[S2S] Warning: sem for epoch %ld not found!\n", *epoch_id);
                } else {
                        sem_post(*sem);
                        printf("[S2S] Post the sem for epoch %ld\n", *epoch_id);
                }*/
        } else {
                //printf("Receievd heartbeat from %s, timestamp = %ld\n", saddr, *timestamp);
                
                long *send_time = (long *)(data + 52);

                pthread_mutex_lock(&lock); 
                long epoch_id = round_to_epoch(*send_time); // find the current epoch to be touched
               
                if(epoch_id < 0)
                        goto ret;

                printf("[Receiver] Heartbeat message from %s for epoch %ld (%ld) sent at %ld received at time %ld.\n", saddr, epoch_id, round_to_interval(epoch_id), *send_time, now());
                /*if(ht_contains(&ht, &epoch_id, sizeof(long))) {
                        ht_remove(&ht, &epoch_id, sizeof(long));
                        printf("[Receiver] Epoch %ld removed.\n", epoch_id);
                } else {
                        printf("[Receiver] Waring: epoch %ld not found when removing!\n", epoch_id);
                }
               
                // add next epoch
                epoch_id ++;
                ht_insert(&ht, &epoch_id, sizeof(long), &epoch_id, sizeof(long)); // the value not matters
                if(ht_contains(&ht, &epoch_id, sizeof(long)))
                        printf("[Receiver] Added epoch %ld as the next epoch.\n", epoch_id);
                else
                        printf("[Receiver] Waring: Filed to add epoch %ld as the next timeout epoch.\n", epoch_id);*/
        } 

ret:
        pthread_mutex_unlock(&lock);
        return id;
}

//void send_to_self(long ) {
//}

// thread for checking expiration
void *expirator(void *arg) {
        base_time = now();
        long next_epoch_id = round_to_epoch(now()); 
        printf("starting expirator, base_time = %ld, expiration_interval = %ld, next_epoch_id = %ld\n", base_time, expiration_interval, next_epoch_id);
        
      /*  while(1) {
                long current_time = now();
                long next_expiration_time = round_to_interval(next_epoch_id);

                if(next_expiration_time > current_time) {
                        struct timespec sleep_ts = sleep_time(next_expiration_time - current_time);
                        nanosleep(&sleep_ts, NULL);
                        continue;
                }             

                if(send_to_self(next_epoch_id) < 0) {
                        fprintf(stderr, "Error: Send to self wrong!\n");
                        break;
                }
                printf("\t[S2S] Self-message for epoch %ld sent at time %ld.\n", next_epoch_id, now());
                
                sem_t *sem_p = (sem_t*) calloc(1, sizeof(sem_t));
                sem_init(sem_p, 0, 0);
                pthread_mutex_lock(&lock); 
                ht_insert(&sem_ht, &next_epoch_id, sizeof(long), &sem_p, sizeof(sem_t*));
                pthread_mutex_unlock(&lock); 
                printf("\t[S2S] Inserted and waiting for sem for epoch %ld.\n", next_epoch_id);
                sem_wait(sem_p);
                
                ht_remove(&sem_ht, &next_epoch_id, sizeof(long));
                free(sem_p);
                printf("\t[S2S] Removed sem for epoch %ld.\n", next_epoch_id);
                
                pthread_mutex_lock(&lock);
                if(!ht_contains(&ht, &next_epoch_id, sizeof(long))) {
                        printf("\t[Expirator] Not timeout at epoch %ld.\n", next_epoch_id);
                } else { 
                        printf("\n\t[Expirator] Timeouted at epoch %ld!!\n!\n", next_epoch_id);
                        //exit(1);
                }
                next_epoch_id ++; // add one maybe not enough
                pthread_mutex_unlock(&lock);
        }*/  
        return NULL;
}

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
	      struct nfq_data *nfa, void *data) {
	u_int32_t id = process_hb(nfa);
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
        
        if(argc != 2) {
                printf("./hb_queue [timeout ms]\n");
                return -1;
        } 
        expiration_interval = atoi(argv[1]);
        
        signal(SIGINT, sig_handler);

        ht_init(&ht, HT_NONE, 0.05);
        ht_init(&sem_ht, HT_NONE, 0.05);
        
        selffd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(SELFIP);
        serv_addr.sin_port = htons(PORT);

        //if(connect(selffd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        //        fprintf(stderr, "Error: connect failed.\n");
        //        return -1;
        //} else {
                printf("[hb_queue] local send-self server connected\n");
                pthread_create(&expirator_tid, NULL, expirator, NULL);
                printf("Expirator created.\n");
       // }

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
