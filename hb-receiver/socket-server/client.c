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

#define MSGSIZE 10
#define TIMEOUT_INTVL 10 // 10s

int sock = 0;

void sig_handler(int signo) {
      if (signo == SIGINT) {
          printf("client ternimates and close conn\n");
          close(sock);
      }
}

struct timespec sleep_time() {
    struct timespec ts;
    ts.tv_sec = TIMEOUT_INTVL;
    ts.tv_nsec = 0;
    return ts;
}

int main(int argc , char *argv[]) {

    signal(SIGINT, sig_handler);
    
    if(argc!=2) {
	printf("client [ip]\n");
	return -1;
    }
    char *ip = argv[1];
    printf("ip = %s, msg_size = %d\n", ip, MSGSIZE);

    struct sockaddr_in server;
    char *buf = malloc(MSGSIZE); 
    sock = socket(AF_INET, SOCK_STREAM , 0);
    if (sock == -1) {
        printf("Could not create socket\n");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(5001);

    if (connect(sock, (struct sockaddr *) &server , sizeof(server)) < 0) {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");
    
    while(1) {   
        int ret = send(sock, buf, MSGSIZE, 0);
        if(ret <= 0) {
            break;
        }
        if(ret != MSGSIZE) 
            printf("warning write ret=%d\n", ret);
       
        // sleep
        struct timespec sleep_ts = sleep_time();
        nanosleep(&sleep_ts, NULL); 
    }

    return 0;
}
