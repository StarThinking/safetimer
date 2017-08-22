#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

long count = 0;

void sig_handler(int signo) {
    printf("count = %ld\n", count);
    exit(0);
}

int main(int argc , char *argv[]) {

    signal(SIGINT, sig_handler);

    if(argc != 5) {
	printf("client [ip] [port] [msg_size] [packets]\n");
	return -1;
    }
    
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int msg_size = atoi(argv[3]);
    int packets_to_send = atoi(argv[4]);
    printf("ip = %s, port = %d, msg_size = %d\n", ip, port, msg_size);

    int _packets_to_send = packets_to_send;
    int sock;
    struct sockaddr_in server, client;
    char *buf = calloc(1, msg_size); 

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock == -1)
    {
        printf("Could not create socket\n");
    }
     
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    int client_len = sizeof(client);
    
    while(1) {   
        int ret = sendto(sock, buf, msg_size, 0, (struct sockaddr *) &server, sizeof(server));
        
        if(ret <= 0) {
            printf("Error: write ret = %d\n", ret);
            break;
        }

        if(ret != msg_size) 
            printf("Warning: write ret = %d\n", ret);
        
        count++;
        _packets_to_send--;
        if(_packets_to_send == 0)
            break;
    }

    close(sock);
    return 0;
}
