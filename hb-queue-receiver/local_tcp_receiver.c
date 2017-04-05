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
#include <semaphore.h>

#define LOCAL_PORT 6001
#define LOCAL_ADDRESS "10.0.0.12"
#define SELF "10.0.0.11"
#define SELF_PORT 5001
#define MSGSIZE sizeof(long)

static int listenfd, connfd;
static pthread_t receiver_tid;

void sig_handler(int signo) {
        if (signo == SIGINT) {
                close(listenfd);
                close(connfd);
                pthread_cancel(receiver_tid);
        }
        exit(0);
}

void *receiver(void *arg) {
        long now_t;
        int ret;
        int connfd = *(int *) arg;
    
        while(1) {
	        ret = recv(connfd, &now_t, MSGSIZE, 0);
	        if(ret <= 0) {
                        fprintf(stderr, "Error: recv ret is less than 0\n");
	                close(connfd);
	                break;
	        }
	        if(ret != MSGSIZE) 
                        printf("Warn: received packet size is %d, not %lu\n", ret, MSGSIZE);
            
                printf("[local_tcp_receiver] local packet received, ret = %d, data = %lu\n", ret, now_t);

                send_to_self();
        }
        free(arg);
}

int main(int argc, char *argv[]) {
        signal(SIGINT, sig_handler);
        
        struct sockaddr_in server, self;
        int connfd;
    
        if(argc != 1) {
                printf("Usage: ./local_tcp_receiver\n");
	        exit(1);
        }

        // local server 10.0.0.12
        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&server, '0', sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(LOCAL_ADDRESS);
        server.sin_port = htons(LOCAL_PORT); 
        bind(listenfd, (struct sockaddr*) &server, sizeof(server)); 
        listen(listenfd, 10); 
    
        // self 10.0.0.11
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = inet_addr(SELF);
        server.sin_port = htons(SELF_PORT); 

        while(1) {
                struct sockaddr_in client;
                unsigned int client_len;
	        pthread_t tid;
        
                client_len = sizeof(client);
                connfd = accept(listenfd, (struct sockaddr*) &client, &client_len);
                int *tmp = malloc(sizeof(int));
	        *tmp = connfd;
	        pthread_create(&tid, NULL, receiver, tmp);
                receiver_tid = tid;
        
                printf("[local_tcp_receiver] new sock connection established, sockfd = %d, ip = %s\n", connfd, inet_ntoa(client.sin_addr));
    }
}
