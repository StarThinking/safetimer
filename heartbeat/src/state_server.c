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

#include "hb_common.h"
#include "state_server.h"

long node_state = 0; // not stale

static pthread_t server_tid; /* Heartbeat server pthread id. */
static int server_fd;

static void *state_server(void *arg);
static void cleanup(void *arg);

int init_state_server() {
        int ret = 0;
        
        struct sockaddr_in server;

        memset(&server, '0', sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(STATE_SERVER_ADDR);
        server.sin_port = htons(STATE_SERVER_PORT);

        server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        bind(server_fd, (struct sockaddr*) &server, sizeof(server));

        /* Start heartbeat server thread. */
        pthread_create(&server_tid, NULL, state_server, NULL);
        
        printf("State server thread started.\n");
       
        return ret;
}

void cancel_state_server() {
        pthread_cancel(server_tid);
}

void join_state_server() {
        pthread_join(server_tid, NULL);
        printf("State server thread joined.\n");
}

static void cleanup(void *arg) {  
        printf("Clean up state server.\n");
        close(server_fd);
}

/*
 * Entry function of state server.
 * Messages consist of two parts: [app_id].
 * If not stale, returns 0; if stale, returns 1.
 */
static void *state_server(void *arg) {
        long msg_buffer[1];
        long app_id;
        struct sockaddr_in client;
        unsigned int len = sizeof(client);

        pthread_cleanup_push(cleanup, NULL);

        while (1) {
                if((recvfrom(server_fd, &msg_buffer, MSGSIZE, 0, (struct sockaddr *) &client, &len)) != MSGSIZE) {
                        perror("recvfrom");
                        break;
                }

                app_id = msg_buffer[0];

                if (app_id >= 0) {
                        /* For requests, reply the state of that app. */
                        long reply[1];

                        reply[0] = node_state;

                        if(sendto(server_fd, reply, MSGSIZE, 0, (struct sockaddr *) &client, 
                                        sizeof(client)) != MSGSIZE) {
                                perror("send reply");
                                break;
                        } 
                        
                        printf("State server: replied node_state %ld to app %ld\n",
                                reply[0], app_id);
                } 
        }

        pthread_cleanup_pop(1);
        return NULL;
}
