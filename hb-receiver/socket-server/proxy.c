#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

#define HB_RECEIVER "10.0.0.12"
#define MSGSIZE sizeof(long)

static pthread_t expire_recv_tid, hb_send_tid;
static int listenfd = 0, connfd = 0, respfd = 0; 
static long timeout_intvl_ms = 0;
static int ret;

void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("client ternimates and close conn\n");
        close(connfd);
        close(listenfd);
        close(respfd);
        pthread_cancel(hb_send_tid);
        pthread_cancel(expire_recv_tid);
        exit(0);
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

void *hb_send(void *arg) {
    long now_t;
    while(1) {
        now_t = now();
        if((ret = send(respfd, &now_t, MSGSIZE, 0)) <= MSGSIZE) {
            printf("[hb_send] Error: send ret = %d\n", ret);
        }
        // sleep
        struct timespec sleep_ts = sleep_time();
        nanosleep(&sleep_ts, NULL);
    }
}

void *expire_recv(void *arg) {
    long msg;
    while(1) {
	if((ret = recv(connfd, &msg, MSGSIZE, 0)) != MSGSIZE) {
            printf("[expire_recv] Error: recv ret = %d\n", ret);
	}
     
        if(msg == 0) { // expire msg
            printf("[expire_recv] receievd expire message from %s\n", HB_RECEIVER);
            if((ret = send(respfd, &msg, MSGSIZE, 0)) != MSGSIZE) // response expire msg 
                printf("[expire_recv] Error: send ret = %d\n", ret);
        } else {
            printf("[expire_recv] Error: wrong expire message value %ld\n", msg);
        }
    }
}

int main(int argc, char *argv[]) {
    
    signal(SIGINT, sig_handler);
    
    if(argc != 2) {
        printf("./proxy [timeout ms]\n");
	return -1;
    }
    timeout_intvl_ms = atoi(argv[1]);

    struct sockaddr_in serv_addr, send_addr;

    // config local server
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5001); 
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 
    listen(listenfd, 10); 

    // config hb receiver
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = inet_addr(HB_RECEIVER);
    send_addr.sin_port = htons(5001); 
    respfd = socket(AF_INET, SOCK_STREAM, 0);
    while(connect(respfd, (struct sockaddr *) &send_addr, sizeof(send_addr)) < 0) {
        perror("connect failed. Error");
        sleep(1);
    } 
    printf("[main] connected to hb receiver %s\n", HB_RECEIVER);
    
    pthread_create(&hb_send_tid, NULL, hb_send, NULL);

    while(1) {
        struct sockaddr_in client;
        unsigned int client_len = sizeof(client);
        connfd = accept(listenfd, (struct sockaddr*) &client, &client_len);
        printf("[main] client connection accepted, sock = %d, ip = %s\n", connfd, inet_ntoa(client.sin_addr));
	pthread_create(&expire_recv_tid, NULL, expire_recv, NULL);
    }
    return 0;
}
