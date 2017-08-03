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

#include "hb_config.h"

static pthread_t udp_server_tid;
static int udp_server_sockfd;

static int tcp_listenfd;
static int conn_num = 0;
static pthread_t tcp_server_tid;
static pthread_t sender_request_receiver_tid;
static pthread_t s2s_msg_receiver_tids[IRQ_NUM];
static int self_connfds[IRQ_NUM];

static int hb_received = 0;

void sig_handler(int signo) {
        pthread_cancel(udp_server_tid);
        pthread_cancel(tcp_server_tid);
        printf("hb_received = %d\n", hb_received);
}

void *udp_server(void *arg) {
        struct sockaddr_in udp_server;
        int ret;
        
        udp_server_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        memset(&udp_server, '0', sizeof(udp_server));
        udp_server.sin_family = AF_INET;
        udp_server.sin_addr.s_addr = inet_addr(LOCAL_IP);
        udp_server.sin_port = htons(HB_PORT); 
        bind(udp_server_sockfd, (struct sockaddr*) &udp_server, sizeof(udp_server)); 

        while(1) {
                long msg;
                struct sockaddr_in client;
                unsigned int client_len = sizeof(client);
                    
                ret = recvfrom(udp_server_sockfd, &msg, MSGSIZE, 0, (struct sockaddr *) &client,
                        &client_len);
                
                if(ret < 0) {
                        fprintf(stderr, "Error: recvfrom ret is less than 0.\n");
	                close(udp_server_sockfd);
	                break;
                }
	        
                if(ret != MSGSIZE) 
                        printf("Warning: received packet size is %d, not %lu !\n", ret, MSGSIZE);
            
                hb_received ++;
                printf("[udp] packet received from %s, ret = %d, data = %ld.\n", 
                        inet_ntoa(client.sin_addr), ret, msg);

        }
        return NULL;
}

void *sender_request_receiver(void *arg) {
        int ret;
        int connfd = *(int *) arg;
        free(arg);

        printf("sender_request_receiver which receives 2 longs\n");

        while(1) {
                FILE *fp;
                char *buf;
                long recv_buf[2];
                long data_to_send[2];
                long base_time, timeout_interval;
                
                ret = recv(connfd, &recv_buf, SELF_MSGSIZE, 0);
                if(ret <= 0) {
                        fprintf(stderr, "Error: recv ret is less than 0\n");
                        close(connfd);
                        break;
                }

                if(ret != SELF_MSGSIZE)
                        printf("Warning: received packet size is %d, not %lu !\n", ret, SELF_MSGSIZE);
                      
                printf("[tcp] packet received, ret = %d, recv_buf[0] = %ld, recv_buf[1] = %ld.\n", 
                        ret, recv_buf[0], recv_buf[1]);
                
                // send back base_time and timeout interval to heartbeat sender
                fp = fopen("/sys/kernel/debug/hb_receiver/base_time", "r");
                buf = (char*) calloc(20, sizeof(char));
                fgets(buf, 20, fp);
                base_time = atol(buf);
                free(buf);
                
                fp = fopen("/sys/kernel/debug/hb_receiver/timeout_interval", "r");
                buf = (char*) calloc(20, sizeof(char));
                fgets (buf, 20, fp);
                timeout_interval = atol(buf);
                free(buf);

                data_to_send[0] = base_time;;
                data_to_send[1] = timeout_interval;
                ret = send(connfd, data_to_send, SELF_MSGSIZE, 0);
                
                if(ret <= 0) {
                        fprintf(stderr, "Error: send ret is 0!\n");
                        exit(1);
                }
                
                if(ret != SELF_MSGSIZE)
                        printf("Warning: send ret is %d.\n", ret);

                printf("[tcp] base_time %ld and timeout_interval %ld has been sent to heartbeat sender\n"
                            ,data_to_send[0], data_to_send[1]);
                close(connfd);
        }
        return NULL;
}

void *s2s_msg_receiver(void *arg) {
        int ret;
        int connfd = *(int *) arg;
        free(arg);

        printf("s2s_msg_receiver which receives 2 longs\n");
        while(1) {
                long data[2];
                ret = recv(connfd, data, SELF_MSGSIZE, 0);
                if(ret <= 0) {
                        fprintf(stderr, "Error: recv ret is less than 0\n");
                        close(connfd);
                        break;
                }

                if(ret != SELF_MSGSIZE)
                        printf("Warning: received packet size is %d, not %lu !\n", ret, SELF_MSGSIZE);
                      
                printf("[tcp] packet received, ret = %d, data[0] = %ld, data[1] = %ld.\n", ret, data[0], data[1]);
        }
        return NULL;
}

char *concat(const char *s1, const char *s2) {
        char *result = malloc(strlen(s1) + strlen(s2) + 1);
        strcpy(result, s1);
        strcat(result, s2);
        return result;
}

int get_sport_of_irq(const int irq) {
        char *r1, *r2, *r3;
        char irq_str[4];
        FILE *fp;
        char buf[8];
        int port;

        sprintf(irq_str, "%d", irq);
        r1 = concat("/sys/kernel/debug/", SELF_IP);
        r2 = concat(r1, "/");
        r3 = concat(r2, irq_str);

        fp = fopen(r3, "r");
        if(fp == NULL) {
                fprintf(stderr, "Error: can't open debugfs file %s!\n", r3);
                exit(1);
        }

        fscanf(fp, "%s", buf);
        port = atoi(buf);

        fclose(fp);
        free(r1);
        free(r2);
        free(r3);

        return port;
}

int sockfd_is_first(struct sockaddr_in client, int *index) {
        int i;
        int sport;

        // retrieve sport of this sockfd
        sport = htons(client.sin_port);

        // if sport is within any irq file, return 1; otherwise, return 0.
        for(i=0; i<IRQ_NUM; i++) {
                int irq = i + BASE_IRQ;
                int sport_read = get_sport_of_irq(irq);
                if(sport_read == sport) {
                        *index = i;
                        return 1;
                }
        }
        
        *index = -1; 
        return 0;
}

void *tcp_server(void *arg) {
        struct sockaddr_in tcp_server;
        
        tcp_listenfd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&tcp_server, '0', sizeof(tcp_server));
        tcp_server.sin_family = AF_INET;
        tcp_server.sin_addr.s_addr = inet_addr(LOCAL_IP);
        tcp_server.sin_port = htons(LOCAL_PORT);
        bind(tcp_listenfd, (struct sockaddr*) &tcp_server, sizeof(tcp_server));
        listen(tcp_listenfd, 10);

        while(1) {
                struct sockaddr_in client;
                int connfd = 0;
	        long msg;
                unsigned int client_len;
                pthread_t tid;
                int index;

                client_len = sizeof(client);
                connfd = accept(tcp_listenfd, (struct sockaddr*) &client, &client_len);

                // Establish #rx-irqs TCP connections from self sender with this local tcp server
                if(strcmp(inet_ntoa(client.sin_addr), SELF_IP) == 0) {
                        recv(connfd, &msg, MSGSIZE, 0);
                
                        if(msg == 0) { // first packet
                                if(!sockfd_is_first(client, &index)) {
                                        send(connfd, &index, MSGSIZE, 0);
                                        close(connfd);
                                        continue;
                                 } else {
                                        self_connfds[index] = connfd;
                                        send(connfd, &index, MSGSIZE, 0);
                                 }
                        }
 
                        int *tmp = malloc(sizeof(int));
                        *tmp = connfd;
                        pthread_create(&tid, NULL, s2s_msg_receiver, tmp);
                        s2s_msg_receiver_tids[conn_num++] = tid;
    
                        printf("New SELF TCP socket connection established, sockfd = %d, ip = %s, sport = %u\n",
                                connfd, inet_ntoa(client.sin_addr), htons(client.sin_port));
                } else {
                        int *tmp = malloc(sizeof(int));
                        *tmp = connfd;
                        pthread_create(&tid, NULL, sender_request_receiver, tmp);
                        sender_request_receiver_tid = tid;
                        
                        printf("New Sender Request TCP socket connection established, sockfd = %d, ip = %s, sport = %u\n",
                                connfd, inet_ntoa(client.sin_addr), htons(client.sin_port));
                        
                }
        }   
}

int main(int argc, char *argv[]) {
        signal(SIGINT, sig_handler);

        if(argc != 1) {
                printf("Usage: ./hb_receiver\n");
	        exit(1);
        }

        pthread_create(&udp_server_tid, NULL, udp_server, NULL);
        pthread_create(&tcp_server_tid, NULL, tcp_server, NULL);
        
        pthread_join(udp_server_tid, NULL);
        pthread_join(tcp_server_tid, NULL);
       
        close(udp_server_sockfd);
        close(tcp_listenfd);
        
        int i;
        for(i = 0; i < conn_num; i++){
                close(self_connfds[i]);
                pthread_cancel(s2s_msg_receiver_tids[i]);
        }
        pthread_cancel(sender_request_receiver_tid);

        printf("Program exits.\n");
        
        return 0;
}
