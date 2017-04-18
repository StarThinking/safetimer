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

#define PORT 5001
#define MSGSIZE sizeof(long)

static int sockfd = 0;
static long timeout_intvl_ms = 0;
static char *receiver_ip = "";

void sig_handler(int signo) {
      if (signo == SIGINT) {
          printf("Ternimate!\n");
          close(sockfd);
      }
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
    
        if(argc != 3) {
	        printf("Usage: ./udp_sender [ip] [timeout ms]\n");
	        return -1;
        }
        receiver_ip = argv[1];
        timeout_intvl_ms = atoi(argv[2]);
        printf("receiver_ip = %s, timeout = %ld ms, msg_size = %lu\n", receiver_ip, timeout_intvl_ms, MSGSIZE);

        sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        server.sin_addr.s_addr = inet_addr(receiver_ip);
        server.sin_family = AF_INET;
        server.sin_port = htons(PORT);

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
       
                // sleep
                struct timespec sleep_ts = sleep_time();
                nanosleep(&sleep_ts, NULL); 
        }
        
        return 0;
}
