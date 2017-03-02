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

//#define MSGSIZE 8
#define MSGSIZE sizeof(long)

static int sock = 0;
static long timeout_intvl_ms = 0;
static char *receiver_ip = "";

void sig_handler(int signo) {
      if (signo == SIGINT) {
          printf("client ternimates and close conn\n");
          close(sock);
      }
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
    
    if(argc != 3) {
	printf("client [ip] [timeout ms]\n");
	return -1;
    }
    receiver_ip = argv[1];
    timeout_intvl_ms = atoi(argv[2]);
    printf("receiver_ip = %s, timeout = %ld ms, msg_size = %lu\n", receiver_ip, timeout_intvl_ms, MSGSIZE);

    struct sockaddr_in server;
    char *buf = malloc(MSGSIZE); 
    sock = socket(AF_INET, SOCK_STREAM , 0);
    if (sock == -1) {
        printf("Could not create socket\n");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr(receiver_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(5001);

    if (connect(sock, (struct sockaddr *) &server , sizeof(server)) < 0) {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");
    
    while(1) {  
        long now_t = now();
        int ret = send(sock, &now_t, sizeof(long), 0);
        //int ret = send(sock, buf, MSGSIZE, 0);
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
