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

#define PORT 5001
#define LOCAL_ADDR "10.0.0.11"
#define MSGSIZE sizeof(long)
#define MAX_CONN_NUM 5
#define RX_RING_IRQ 51

static int listenfd, conn_num = 0;
static pthread_t receiver_tids[MAX_CONN_NUM];
static int connfds[MAX_CONN_NUM];
static int localsenderfn;

void sig_handler(int signo) {
        int i;
        if (signo == SIGINT) {
                close(listenfd);
                for(i = 0; i < conn_num; i++){
                    close(connfds[i]);
                    pthread_cancel(receiver_tids[i]);
                }
                exit(0);
        }
}

char* concat(const char *s1, const char *s2) {
        char *result = malloc(strlen(s1) + strlen(s2) + 1);
        strcpy(result, s1);
        strcat(result, s2);
        return result;
}

int get_irq(struct sockaddr_in *client) {
        FILE *fp;
        char buf;
        int irq;
        char *ip_addr = inet_ntoa(client->sin_addr);
        char *r1, *r2;
        
        r1 = concat("/sys/kernel/debug/", ip_addr);
        r2 = concat(r1, "/irq");
        fp = fopen(r2, "r");
        if(fp == NULL) {
                fprintf(stderr, "Error: Can't open input file %s!\n", r2);
                exit(1);
        }
        fscanf(fp, "%s", &buf);
        irq = atoi(&buf);
        printf("irq = %d\n", irq); 
        fclose(fp);
        free(r1);
        free(r2);

        return irq;
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
                        printf("Warn: received packet size is %d, not %lu\n", ret, MSGSIZE);
            
                printf("[tcp_receiver] packet received, ret = %d, data = %lu\n", ret, now_t);
        }
        free(arg);
}

int main(int argc, char *argv[]) {
        signal(SIGINT, sig_handler);
    
        struct sockaddr_in server;
    
        if(argc != 2) {
                printf("Usage: ./receiver [timeout ms]\n");
	        exit(1);
        }

        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&server, '0', sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(LOCAL_ADDR);
        server.sin_port = htons(PORT); 
        bind(listenfd, (struct sockaddr*) &server, sizeof(server)); 
        listen(listenfd, 10); 
    
        long msg;
        const long irq_not_ok = 0;
        const long irq_ok = 1;
        int irq, ret, connfd;
        while(1) {
                struct sockaddr_in client;
                unsigned int client_len;
                pthread_t tid;

                client_len = sizeof(client);
                connfd = accept(listenfd, (struct sockaddr*) &client, &client_len);
                
                irq = get_irq(&client);
                printf("irq = %d\n", irq);
                ret = recv(connfd, &msg, MSGSIZE, 0);
                if(msg == 0) { // first packet
                        if(irq != RX_RING_IRQ) {
                                send(connfd, &irq_not_ok, MSGSIZE, 0);
                                printf("IRQ is %d not %d. Close connfd and reply 0\n", irq, RX_RING_IRQ);
                                close(connfd);
                                continue;
                        } else {
                                send(connfd, &irq_ok, MSGSIZE, 0);
                                printf("IRQ = RX_RING_IRQ = %d.\n", RX_RING_IRQ);
                        }
                }

                int *tmp = malloc(sizeof(int));
                *tmp = connfd;
	        pthread_create(&tid, NULL, receiver, tmp);
                receiver_tids[conn_num++] = tid;
        
                printf("[tcp_receiver] new sock connection established, sockfd = %d, ip = %s, sport = %u\n", connfd, inet_ntoa(client.sin_addr), htons(client.sin_port));
    }
}
