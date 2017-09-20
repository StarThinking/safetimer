#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <signal.h>

#include "helper.h"

#define HB_SERVER_ADDR "10.0.0.11"
#define HB_SERVER_PORT 6789
#define MSGSIZE sizeof(long)

long base_time;
long timeout_interval = 1000;

static pthread_t server_tid; /* Heartbeat server pthread id. */
static int server_fd;

static int init_receiver();
static void *hb_server(void *arg);
static void destroy_receiver();
static void cleanup(void *arg);
static void cancel_receiver();
static void join_receiver();

static int init_receiver() {
        int ret = 0;
        
        struct sockaddr_in server;

        base_time = now_time();

        memset(&server, '0', sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(HB_SERVER_ADDR);
        server.sin_port = htons(HB_SERVER_PORT);

        server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        bind(server_fd, (struct sockaddr*) &server, sizeof(server));

        /* Start heartbeat server thread. */
        pthread_create(&server_tid, NULL, hb_server, NULL);
        
        printf("NHeartbeat receiver thread started.\n");
       
        return ret;
}

/*
 * Entry function of heartbeat server.
 * Messages consist of two parts: [flag, epoch_id].
 * If flag is 0, it indicates a request message;
 * If flag is 1, it indicates a heartbeat message.
 */
static void *hb_server(void *arg) {
        long msg_buffer[2];
        long flag;
        struct sockaddr_in client;
        unsigned int len = sizeof(client);

        pthread_cleanup_push(cleanup, NULL);

        while (1) {
                if((recvfrom(server_fd, &msg_buffer, MSGSIZE*2, 0, (struct sockaddr *) &client, &len)) != MSGSIZE*2) {
                        perror("recvfrom");
                        break;
                }
                
                flag = msg_buffer[0];

                if (flag == 0) {
                        /* For requests, reply [base_time]. */
                        long reply = base_time;

                        if(sendto(server_fd, &reply, MSGSIZE, 0, (struct sockaddr *) &client,
                                    sizeof(client)) != MSGSIZE) {
                                perror("send reply");
                                break;
                         }
                } else if (flag == 1) {
                        /* Heartbeats. */
                        long epoch_id;
                        epoch_id = msg_buffer[1];
                    
                        printf("NHeartbeat receiver: heartbeat from %s:%u for epoch %ld.\n",
                                inet_ntoa(client.sin_addr), ntohs(client.sin_port), epoch_id);
                }
        }

        pthread_cleanup_pop(1);
        return NULL;
}

static void sig_handler(int signo) {
        destroy_receiver();
        printf("NHeartbeat receiver ternimates in sig_handler.\n");
        exit(0);
}

static void cancel_receiver() {
        pthread_cancel(server_tid);
}

static void join_receiver() {
        pthread_join(server_tid, NULL);
        printf("NHeartbeat receiver thread joined.\n");
}

static void cleanup(void *arg) {  
        printf("Clean up NHeartbeat receiver.\n");
        close(server_fd);
}

static void destroy_receiver() {
        cancel_receiver();
        join_receiver();
        printf("NHeartbeat receiver has been destroyed.\n");
}

int main(int argc, char *argv[]) {
        signal(SIGINT, sig_handler);
        signal(SIGTERM, sig_handler);
        signal(SIGKILL, sig_handler);
         
        if (init_receiver() == 0) {
                while(1) {
                        sleep(1);
                }
        }
            
        destroy_receiver();

        return 0;
}
