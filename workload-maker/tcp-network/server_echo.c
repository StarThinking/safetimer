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

int msg_size = 0;


void * socket_thread(void *arg){
    char *buf = malloc(msg_size);
    int connfd = *(int *) arg;
    while(1){
	int ret = recv(connfd, buf, msg_size, 0);
	if(ret <= 0){
	    close(connfd);
	    break;
	}
	if(ret != msg_size) printf("warning recv ret=%d\n", ret);
        ret = send(connfd, buf, ret, 0);
	if(ret <= 0){
            close(connfd);
            break;
        }
        if(ret != msg_size) printf("warning write ret=%d\n", ret);
    }
        
}

int main(int argc, char *argv[])
{
    if(argc!=2){
	printf("server [msg_size]\n");
	return -1;
    }
    msg_size = atoi(argv[1]);
    printf("msg_size = %d\n", msg_size);
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr; 


    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

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
