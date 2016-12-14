#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

long current_time ()
{
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec/1.0e6;
}
 
int main(int argc , char *argv[])
{

    if(argc!=4){
	printf("client [ip] [msg_size] [time in ms]\n");
	return -1;
    }
    char *ip = argv[1];
    int msg_size = atoi(argv[2]);
    int time = atoi(argv[3]);
    printf("ip=%s, msg_size=%d, time=%d\n", ip, msg_size, time);

    int sock;
    struct sockaddr_in server;
    char *buf = malloc(msg_size); 

    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);

    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");

    long start_time = current_time();
    long count = 0;

    while(current_time()-start_time < time)
    {   
        int ret = send(sock, buf, msg_size, 0);
        if(ret <= 0){
            break;
        }
        if(ret != msg_size) printf("warning write ret=%d\n", ret);
	ret = recv(sock, buf, msg_size, 0);
        if(ret <= 0){
            break;
        }
        if(ret != msg_size) printf("warning recv ret=%d\n", ret);
	count++;
    }
    printf("thoughput=%f\n", (double)count*(double)msg_size/(double)(current_time()-start_time));
    close(sock);
    return 0;
}
