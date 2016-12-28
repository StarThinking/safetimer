#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

long current_time()
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
    struct sockaddr_in server, client;
    char *buf = malloc(msg_size); 

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock == -1)
    {
        printf("Could not create socket\n");
    }
     
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);

    long start_time = current_time();
    long count = 0;
    int client_len = sizeof(client);
    while(current_time()-start_time < time)
    {   
        int ret = sendto(sock, buf, msg_size, 0, (struct sockaddr *) &server, sizeof(server));
        if(ret <= 0) {
            break;
        }
        if(ret != msg_size) printf("warning write ret=%d\n", ret);
        
	ret = recvfrom(sock, buf, msg_size, 0, (struct sockaddr *) &client, &client_len);
        if(ret <= 0){
            break;
        }
        if(ret != msg_size) printf("warning recv ret=%d\n", ret);

        count++;
    }
    double thput = (double)count*(double)msg_size/(double)(current_time()-start_time);
    printf("thoughput=%d kb/s\n", (int)thput);
    close(sock);
    return 0;
}
