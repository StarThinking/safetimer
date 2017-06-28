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

#include "hb_config.h"

static int sockfd = 0;
static long timeout_intvl_ms = 0;
static char *receiver_ip = "";

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

        struct sockaddr_in server, local_server;
        int _sockfd;
        int ret;
        long request[2];
        long recv_buf[2];
    
        if(argc != 3) {
	        printf("Usage: ./udp_sender [ip] [timeout ms]\n");
	        return -1;
        }
        receiver_ip = argv[1];
        timeout_intvl_ms = atoi(argv[2]);
        printf("receiver_ip = %s, timeout = %ld ms, msg_size = %lu\n", receiver_ip, timeout_intvl_ms, MSGSIZE);

        // establish connections with local server
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        local_server.sin_family = AF_INET;
        local_server.sin_addr.s_addr = inet_addr(LOCAL_IP);
        local_server.sin_port = htons(LOCAL_PORT);

        if(connect(_sockfd, (struct sockaddr *) &local_server, sizeof(local_server)) < 0) {
                fprintf(stderr, "Error: connect failed.\n");
                exit(1);
        }
        printf("The TCP connection with local server have been %s established.\n", LOCAL_IP);
        
        request[0] = 0;
        request[1] = 0;
        ret = send(_sockfd, request, SELF_MSGSIZE, 0);
        if(ret <= 0) {
                fprintf(stderr, "Error: send ret is 0!\n");
                exit(1);
        }
        
        if(ret != SELF_MSGSIZE)
                printf("Warning: send ret is %d.\n", ret);
       
        printf("hb prep info request sent\n");

        ret = recv(_sockfd, recv_buf, SELF_MSGSIZE, 0);
        if(ret <= 0) {
                fprintf(stderr, "Error: recv ret is less than 0\n");
                close(_sockfd);
                exit(1);
        }
           
        if(ret != SELF_MSGSIZE)
                printf("Warning: received packet size is %d, not %lu !\n", ret, SELF_MSGSIZE);

        printf("packet received, ret = %d, base_time = %ld, timeout_interval = %ld.\n", 
                ret, recv_buf[0], recv_buf[1]);

        // UDP
        sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        server.sin_addr.s_addr = inet_addr(receiver_ip);
        server.sin_family = AF_INET;
        server.sin_port = htons(HB_PORT);

        long begin_t = now();
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
                
                packet_sent ++;
                if(packet_sent == 10000)
                        break;
                // sleep
                struct timespec sleep_ts = sleep_time();
                nanosleep(&sleep_ts, NULL); 
        }
        
        long end_t = now();
        printf("Time used to send 10000 packets is %ld\n", end_t - begin_t);
        return 0;
}
