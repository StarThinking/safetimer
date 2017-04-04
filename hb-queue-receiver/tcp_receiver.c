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

#define PORT 5001
#define LOCAL_SENDER "10.0.0.12"
#define MSGSIZE sizeof(long)
#define MAX_CONN_NUM 5

static int listenfd, conn_num;
static pthread_t receiver_tids[MAX_CONN_NUM];
static int connfds[MAX_CONN_NUM];
static int localsenderfn;

void sig_handler(int signo) {
        int i;
        if (signo == SIGINT) {
                close(listenfd);
                for(i = 0; i < conn_num; i++){
                    close(connfds[i]);
                    pthread_cancel(receiver_tids[i]);
                }
                exit(0);
        }
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
            
                printf("[tcp_receiver] packet received, ret = %d, data = %lu\n", ret, now_t);
        }
}

int main(int argc, char *argv[]) {
        signal(SIGINT, sig_handler);
    
        struct sockaddr_in server;
        int connfd;
    
        if(argc != 2) {
                printf("Usage: ./receiver [timeout ms]\n");
	        exit(1);
        }

        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&server, '0', sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        server.sin_port = htons(PORT); 
        bind(listenfd, (struct sockaddr*) &server, sizeof(server)); 
        listen(listenfd, 10); 
    
        conn_num = 0;
        while(1) {
                struct sockaddr_in client;
                unsigned int client_len;
                pthread_t tid;
                int *tmp = malloc(sizeof(int));

                client_len = sizeof(client);
                connfd = accept(listenfd, (struct sockaddr*) &client, &client_len);
	        *tmp = connfd;
	        pthread_create(&tid, NULL, receiver, tmp);
                receiver_tids[conn_num++] = tid;
        
                printf("[tcp_receiver] new sock connection established, sockfd = %d, ip = %s, sport = %u\n", connfd, inet_ntoa(client.sin_addr), htons(client.sin_port));
    }
}
