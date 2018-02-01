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
#include "hb_server.h"

static pthread_t server_tid; /* Heartbeat server pthread id. */
static int server_fd;

static void *hb_server(void *arg);
static void cleanup(void *arg);

int init_hb() {
        int ret = 0;
        
        struct sockaddr_in server;

        memset(&server, '0', sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(HB_SERVER_ADDR);
        server.sin_port = htons(HB_SERVER_PORT);

        server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        bind(server_fd, (struct sockaddr*) &server, sizeof(server));

        /* Start heartbeat server thread. */
        pthread_create(&server_tid, NULL, hb_server, NULL);
        
        printf("Heartbeat server thread started.\n");
       
        return ret;
}

void cancel_hb() {
        pthread_cancel(server_tid);
}

void join_hb() {
        pthread_join(server_tid, NULL);
        printf("Heartbeat server thread joined.\n");
}

static void cleanup(void *arg) {  
        printf("Clean up heartbeat server.\n");
        close(server_fd);
}

static void *hb_server(void *arg) {
        long msg_buffer;
        struct sockaddr_in client;
        unsigned int len = sizeof(client);

        pthread_cleanup_push(cleanup, NULL);

        while (1) {
                if((recvfrom(server_fd, &msg_buffer, MSGSIZE*1, 0, (struct sockaddr *) &client, &len)) != MSGSIZE*1) {
                        perror("recvfrom");
                        break;
                }
                        
                printf("Heartbeat server: heartbeat from %s:%u with meesage %ld.\n",
                                inet_ntoa(client.sin_addr), ntohs(client.sin_port), msg_buffer);
        }

        pthread_cleanup_pop(1);
        return NULL;
}
