#include <stdio.h> 
#include <string.h>    
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define MSGSIZE 8

static int sockfd = 0;
static long timeout_intvl_ms = 0;
static char *receiver_ip = "";
static int HB_PORT;
static int packet_sent = 0;

void sig_handler(int signo) {
      //if (signo == SIGINT) {
      printf("Ternimate!\n");
      close(sockfd);      
      printf("The number of packets sent is %d\n", packet_sent);
      exit(0);
}

struct timespec sleep_time() {
        struct timespec ts;
        ts.tv_sec = timeout_intvl_ms / 1000;
        ts.tv_nsec = (timeout_intvl_ms % 1000) * 1.0e6;
        return ts;
}

long now() {
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        return spec.tv_sec * 1000 + spec.tv_nsec/1.0e6;
}

int main(int argc , char *argv[]) {
        signal(SIGINT, sig_handler);

        struct sockaddr_in server;
        int packet_to_send, _packet_to_send;
        int nosleep;

        if(argc != 6) {
	        printf("Usage: ./udp_sender [ip] [port] [timeout ms] [count] [nosleep]\n");
	        return -1;
        }
        receiver_ip = argv[1];
        HB_PORT = atoi(argv[2]);
        timeout_intvl_ms = atoi(argv[3]);
        packet_to_send = atoi(argv[4]);
        nosleep = atoi(argv[5]);
        printf("receiver_ip = %s, timeout = %ld ms, msg_size = %lu, count = %d, nosleep = %d\n", receiver_ip, timeout_intvl_ms, MSGSIZE, packet_to_send, nosleep);

        sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        server.sin_addr.s_addr = inet_addr(receiver_ip);
        server.sin_family = AF_INET;
        server.sin_port = htons(HB_PORT);

        long begin_t = now();
        _packet_to_send = packet_to_send;
        while(1) {  
                long now_t = now();
                int ret = sendto(sockfd, &now_t, MSGSIZE, 0, (struct sockaddr *) &server, sizeof(server));
                
                if(ret <= 0) {
                        fprintf(stderr, "Error: sendto ret is less than 0.\n");
                        close(sockfd);
                        break;
                }
        
                if(ret != MSGSIZE) 
                        printf("Warning: sendto ret is %d!\n", ret);
                
                _packet_to_send --;
                if(_packet_to_send == 0)
                        break;
                
                if(nosleep) {
                        // sleep
                       struct timespec sleep_ts = sleep_time();
                       nanosleep(&sleep_ts, NULL); 
                }
        }
        
        long end_t = now();
        printf("Time used to send %d packets is %ld\n", packet_to_send, end_t - begin_t);
        return 0;
}
