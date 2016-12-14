#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include<signal.h>

#define MSGSIZE 1024
#define INETRVAL 200*1000*1000 // 200ms

int sock;
void sig_handler(int signo)
{
      if (signo == SIGINT) {
          printf("client ternimates and close conn\n");
          close(sock);
      }
}

int main(int argc , char *argv[])
{
    signal(SIGINT, sig_handler);

    if(argc!=2){
	printf("client [ip]\n");
	return -1;
    }
    char *ip = argv[1];
    printf("ip=%s, msg_size=%d\n", ip, MSGSIZE);

    struct sockaddr_in server;
    char *buf = malloc(MSGSIZE); 

    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(5001);

    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");
    
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = INETRVAL; //200ms

    while(1)
    {   
        int ret = send(sock, buf, MSGSIZE, 0);
        if(ret <= 0){
            break;
        }
        if(ret != MSGSIZE) printf("warning write ret=%d\n", ret);
	ret = recv(sock, buf, MSGSIZE, 0);
        if(ret <= 0){
            break;
        }
        if(ret != MSGSIZE) printf("warning recv ret=%d\n", ret);
        
        nanosleep(&ts, NULL); // sleep 
    }
    return 0;
}
