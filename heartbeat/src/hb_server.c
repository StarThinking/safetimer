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

#include "hb_config.h"
#include "hb_server.h"

static pthread_t server_tid; /* Heartbeat server pthread id */
static int server_fd;

static void *hb_server(void *arg);

/*
 * Initialize heartbeat server.
 */
int init_hb_server() {
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

        printf("Heartbeat server has been initialized successfully.\n");

        return 0;
}

/*
 * Clear up all the resources used by heartbeat server.
 */
void destroy_hb_server() {
        close(server_fd);

        pthread_cancel(server_tid);
        pthread_join(server_tid, NULL);
        
        printf("Heartbeat server has been destroyed.\n");
}
/*
 * Entry function of heartbeat server.
 * Message contains epoch id.
 */
static void *hb_server(void *arg) {
        long epoch_id;
        struct sockaddr_in client;
        unsigned int len = sizeof(client);
        
        while (1) {
                if((recvfrom(server_fd, &epoch_id, MSGSIZE, 0, (struct sockaddr *) &client,&len)) != MSGSIZE) {
                        perror("recvfrom");
                        break;
                }

                printf("Heartbeat server: heartbeat from %s:%u for epoch %ld.\n",
                            inet_ntoa(client.sin_addr), ntohs(client.sin_port), epoch_id);
        }

        return NULL;
}
