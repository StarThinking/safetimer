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

#define MSGSIZE 1024
#define BILLION 1000000000L
#define INETRVAL 200*1000*1000 // 200ms

struct timespec current_t, last_t;
uint64_t interval;

void * socket_thread(void *arg) {
    char *buf = malloc(MSGSIZE);
    int connfd = *(int *) arg;
    while(1) {
	int ret = recv(connfd, buf, MSGSIZE, 0);
	if(ret <= 0){
	    close(connfd);
	    break;
	}
        clock_gettime(CLOCK_MONOTONIC, &current_t);
        interval = BILLION * (current_t.tv_sec - last_t.tv_sec) + current_t.tv_nsec - last_t.tv_nsec;
        last_t = current_t;
        printf("[msx] interval between two hbs is %lu ns, latency is %lu ns\n", interval, interval-INETRVAL);
	if(ret != MSGSIZE) printf("warning recv ret=%d\n", ret);
	ret = send(connfd, buf, ret, 0);
	if(ret <= 0){
            close(connfd);
            break;
        }
        if(ret != MSGSIZE) printf("warning write ret=%d\n", ret);
    }
        
}

int main(int argc, char *argv[])
{
    if(argc!=1){
	printf("server\n");
	return -1;
    }
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr; 

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5001); 

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(listenfd, 10); 

    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 
	int *tmp = malloc(sizeof(int));
	*tmp = connfd;
	pthread_t pid;
	pthread_create(&pid, NULL, socket_thread, tmp);
     }
}
