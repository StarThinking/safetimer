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

int main(int argc, char *argv[])
{
    if(argc!=2){
	printf("server [msg_size]\n");
	return -1;
    }
    msg_size = atoi(argv[1]);
    printf("msg_size = %d\n", msg_size);
    int sock = 0;
    int client_len;
    struct sockaddr_in server, client; 
    void *buf = malloc(msg_size);
    int ret;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset(&server, '0', sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("10.0.0.12");
    server.sin_port = htons(5005); 

    bind(sock, (struct sockaddr*) &server, sizeof(server)); 

    while(1) {
        client_len = sizeof(client);

        ret = recvfrom(sock, buf, msg_size, 0, (struct sockaddr *) &client, &client_len);
    
        if(ret < 0) {
            fprintf(stderr, "Error: recvfrom ret is less than 0.\n");
            close(sock);
            break;
        }   
    
        if(ret != msg_size) 
            printf("Warning: received packet size is %d, not %d !\n", ret, msg_size);
    
        printf("received packet from %s\n", inet_ntoa(client.sin_addr));
    }
    free(buf);
    close(sock);
}
