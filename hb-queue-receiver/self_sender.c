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
#define SELFMSG 1111
#define MSGSIZE sizeof(long)

static int listenfd, connfd, selffd;
static pthread_t receiver_tid;

void sig_handler(int signo) {
        if (signo == SIGINT) {
                close(listenfd);
                close(connfd);
                close(selffd);
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
                        printf("Warn: Received packet size is %d, not %lu\n", ret, MSGSIZE);
            
                printf("[self_sender] Local packet received, ret = %d, data = %lu\n", ret, now_t);

                // send to self
                ret = send(selffd, &now_t, MSGSIZE, 0);
                if(ret <= 0)
                        break;
                
                if(ret != MSGSIZE)
                        printf("Warning: Write ret=%d\n", ret);
                printf("[self_sender] Self msg sent.\n");
        }
        free(arg);
}

int verify_rx_ring(const int sockfn) {
        long msg = 0, reply = 0;
        send(sockfn, &msg, MSGSIZE, 0);
        recv(sockfn, &reply, MSGSIZE, 0);
        return reply;
}

int connect_to_desired_ring(struct sockaddr_in server) {
        int _sockfn;
        while(1) {
                if((_sockfn = socket(AF_INET, SOCK_STREAM, 0)) < 0)  
                        fprintf(stderr, "Error: Could not create socket.\n");

                if(connect(_sockfn, (struct sockaddr *) &server, sizeof(server)) < 0)  
                        fprintf(stderr, "Error: Connect failed.\n");
    
                if(!verify_rx_ring(_sockfn)) {
                        printf("[self_sender] Not desired rx ring. Close socket and reconnect.\n");
                        close(_sockfn);
                        sleep(1);
                } else
                        break;
        }   
        return _sockfn; 
}

int main(int argc, char *argv[]) {
        signal(SIGINT, sig_handler);
        
        struct sockaddr_in server, self;
        int connfd;
    
        if(argc != 1) {
                printf("Usage: ./self_sender\n");
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
        self.sin_family = AF_INET;
        self.sin_addr.s_addr = inet_addr(SELF);
        self.sin_port = htons(SELF_PORT); 
        selffd = connect_to_desired_ring(self);
        printf("[self_sender] Connected to self succesfully!\n");

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
        
                printf("[self_sender] New sock connection established, sockfd = %d, ip = %s\n", connfd, inet_ntoa(client.sin_addr));
    }
}
