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

#include "hashtable.h"
#include "hb_common.h"
#include "request_server.h"
#include "sender.h"

static pthread_t server_tid; /* Heartbeat server pthread id. */
static int server_fd;

static void *request_server(void *arg);
static void cleanup(void *arg);

int init_request_server() {
        int ret = 0;
        struct sockaddr_in server;
        
        memset(&server, '0', sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        server.sin_port = htons(STATE_SERVER_PORT);

        server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        bind(server_fd, (struct sockaddr*) &server, sizeof(server));

        /* Start heartbeat server thread. */
        pthread_create(&server_tid, NULL, request_server, NULL);
        
        printf("State server thread started.\n");
       
        return ret;
}

void cancel_request_server() {
        pthread_cancel(server_tid);
}

void join_request_server() {
        pthread_join(server_tid, NULL);
        printf("Request server thread joined.\n");
}

static void cleanup(void *arg) {  
        printf("Clean up request server.\n");
        close(server_fd);
}

static void *request_server(void *arg) {
        long msg_buffer;
        long reply;
        struct sockaddr_in client;
        unsigned int len = sizeof(client);

        pthread_cleanup_push(cleanup, NULL);

        while (1) {
                if((recvfrom(server_fd, &msg_buffer, MSGSIZE, 0, \
                                (struct sockaddr *) &client, &len)) != MSGSIZE) {
                        perror("recvfrom");
                        break;
                }
                printf("Request received, msg = %ld\n", msg_buffer);

                safetimer_send_heartbeat();
                
                reply = 0;
                if(sendto(server_fd, &reply, MSGSIZE, 0, (struct sockaddr *) &client, \
                            sizeof(client)) != MSGSIZE) {
                        perror("send reply");
                        break;
                } 
                
                // to send heartbeat

        }

        pthread_cleanup_pop(1);
        return NULL;
}
