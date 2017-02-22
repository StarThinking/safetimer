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

#define SENDSELFIP "10.0.0.12"
#define MSGSIZE 8

static int sendself_sock;
static int sock; //sockfd used to send back to sendself sock

void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("client ternimates and close conn\n");
        close(sock);
      }
}

void *receiver(void *arg) {
    char *buf = malloc(MSGSIZE);
    int connfd = *(int *) arg;
    int ret;
    while(1) {
	ret = recv(connfd, buf, MSGSIZE, 0);
	if(ret <= 0) {
	    close(connfd);
	    break;
	}
	if(ret != MSGSIZE) printf("warning recv ret=%d\n", ret);
        
        if(connfd == sendself_sock) { // packets from sendself_sock
            printf("[receiver] receievd sendself message from %s\n", SENDSELFIP);
           
            ret = send(sock, buf, MSGSIZE, 0);
            if(ret <= 0) {
                break;
            }
            if(ret != MSGSIZE)
                printf("warning write ret=%d\n", ret);
        }
    }
}

int main(int argc, char *argv[]) {
    
    //signal(SIGINT, sig_handler);
    
    if(argc != 1) {
        printf("./proxy\n");
	return 1;
    }
    
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr, sendself_addr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5001); 
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 
    listen(listenfd, 10); 

    sendself_addr.sin_family = AF_INET;
    sendself_addr.sin_addr.s_addr = inet_addr(SENDSELFIP);
    sendself_addr.sin_port = htons(5001); 

    sock = socket(AF_INET, SOCK_STREAM, 0);
    while(connect(sock, (struct sockaddr *) &sendself_addr, sizeof(sendself_addr)) < 0) {
        perror("connect failed. Error");
        sleep(1);
        //return 1;
    } 
    printf("[main] connected to sendself sock\n");
    
    while(1) {
        struct sockaddr_in client;
        unsigned int client_len = sizeof(client);
        connfd = accept(listenfd, (struct sockaddr*) &client, &client_len);
        printf("[main] client connection accepted, sock = %d, ip = %s\n", connfd, inet_ntoa(client.sin_addr));
        // determine which is the local client sockfd
        if(strcmp(inet_ntoa(client.sin_addr), SENDSELFIP) == 0) {
            sendself_sock = connfd;
        }
	int *tmp = malloc(sizeof(int));
	*tmp = connfd;
	pthread_t receiver_tid;
	pthread_create(&receiver_tid, NULL, receiver, tmp);
    }
    return 0;
}
