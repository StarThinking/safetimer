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

#include "hb_config.h"

static int self_listenfd, self_connfd;
static int local_sockfds[IRQ_NUM];
static pthread_t self_receiver_tid;

void sig_handler(int signo) {
        int i;
        if (signo == SIGINT) {
                for(i=0; i<IRQ_NUM; i++) {
                        close(local_sockfds[i]);
                }
                close(self_listenfd);
                close(self_connfd);
                pthread_cancel(self_receiver_tid);
        }
        exit(0);
}

void *receiver(void *arg) {
        long epoch_id;
        int ret;
        int connfd = *(int *) arg;

        while(1) {
                int i;

	        ret = recv(connfd, &epoch_id, MSGSIZE, 0);
	        if(ret <= 0) {
                        fprintf(stderr, "Error: recv ret is less than 0\n");
	                close(connfd);
	                break;
	        }
	        if(ret != MSGSIZE) 
                        printf("Warn: Received packet size is %d, not %lu\n", ret, MSGSIZE);
            
                printf("[self_sender] Local packet received, ret = %d, epoch_id = %lu\n", ret, epoch_id);

                // send to LOCAL
                for(i=0; i<IRQ_NUM; i++) {
                        long *data = (long*)calloc(2, sizeof(long));
                        int sockfd = local_sockfds[i];

                        data[0] = epoch_id;
                        data[1] = i;
                        ret = send(sockfd, data, SELF_MSGSIZE, 0);
                        if(ret <= 0) {
                                fprintf(stderr, "Error: send ret is 0!\n");
                                exit(1);
                        }
                
                        if(ret != SELF_MSGSIZE)
                                printf("Warning: send ret is %d.\n", ret);

                        free(data);
                }
                printf("[self_sender] All %d self messages for epoch %ld have been sent.\n", IRQ_NUM, epoch_id);
        }
        free(arg);
        return NULL;
}

int validate(const int sockfn) {
        long msg = 0, index = 0;
        send(sockfn, &msg, MSGSIZE, 0);
        recv(sockfn, &index, MSGSIZE, 0);
        return index;
}

void connect_to_all_rings(struct sockaddr_in server) {
        int connected_num = 0;
        int index;
        int sockfd;
        
        while(1) {
                sockfd = socket(AF_INET, SOCK_STREAM, 0);

                if(connect(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) { 
                        fprintf(stderr, "Error: connect failed.\n");
                        exit(1);
                }
                
                index = validate(sockfd);
                if(index >= 0) {
                        printf("[self_sender] This connection is valid and local_sockfds[%d] is %d\n", index, sockfd);
                        local_sockfds[index] = sockfd;
                        connected_num ++;
                } else {
                        printf("[self_sender] The RX ring of this connection is occupied. So, close connection.\n");
                        close(sockfd);
                        sleep(1);
                }
            
                if(connected_num == 4)
                        break;
        }
}

int main(int argc, char *argv[]) {
        signal(SIGINT, sig_handler);
        
        struct sockaddr_in local_server, self_server;
    
        if(argc != 1) {
                printf("Usage: ./self_sender\n");
	        exit(1);
        }

        // establish connections with local server
        local_server.sin_family = AF_INET;
        local_server.sin_addr.s_addr = inet_addr(LOCAL_IP);
        local_server.sin_port = htons(LOCAL_PORT); 
        connect_to_all_rings(local_server);
        printf("[self_sender] All %u connections with local server have been %s established.\n", IRQ_NUM, LOCAL_IP);

        // bind and listen on SELF_IP and SELF_PORT
        self_listenfd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&self_server, '0', sizeof(self_server));
        self_server.sin_family = AF_INET;
        self_server.sin_addr.s_addr = inet_addr(SELF_IP);
        self_server.sin_port = htons(SELF_PORT); 
        bind(self_listenfd, (struct sockaddr*) &self_server, sizeof(self_server)); 
        listen(self_listenfd, 10); 
        
        while(1) {
                struct sockaddr_in client;
                unsigned int client_len = sizeof(client);
                int connfd;

                // there should be only one connection 
                connfd = accept(self_listenfd, (struct sockaddr*) &client, &client_len);
                int *tmp = malloc(sizeof(int));
	        *tmp = connfd;
	        pthread_create(&self_receiver_tid, NULL, receiver, tmp);
        
                printf("[self_sender] New sock connection established, sockfd = %d, ip = %s\n", connfd, inet_ntoa(client.sin_addr));
        }
}
