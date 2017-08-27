#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "hb_config.h"

#define BARRIER_SENDER_INTERFACE "em1"

static pthread_t server_tid; /* Barrier server pthread id */
struct sockaddr_in server;

static int client_sd; /* Barrier client socket descriptors */
static int listen_sd;
static int conn_sd;

static void *barrier_server(void *arg);
static int setup_ring_flows();

int init_barrier() {
        int ret;
        
        pthread_create(&server_tid, NULL, barrier_server, NULL);
        
        sleep(1);

        ret = setup_ring_flows();

        if(ret == 0)
                printf("setup_ring_flows are successful.\n");
        else {
                printf("failed to setup_ring_flows with ret %d.\n", ret);
                return -1;
        }
        return 0;
}

void wait_barrier() {
        pthread_join(server_tid, NULL);
}

void destroy_barrier() {
        close(client_sd);
        close(listen_sd);
        close(conn_sd);
        pthread_cancel(server_tid);
}

int send_barrier_msg() {
        int msg_size;
        int ret = 0;
        long msg_to_send[2];

        /* Send a barrier message */
        msg_size = 2 * MSGSIZE;
        msg_to_send[0] = 123;
        msg_to_send[1] = 456;

        ret = send(client_sd, msg_to_send, msg_size, 0);
        if(ret != msg_size) {
                fprintf(stderr, "Error in send() %s\n", strerror(errno));
                close(client_sd);
                goto error;
        }

        printf("msg is sent.\n");

error:
        return ret;
}

static void *barrier_server(void *arg) {
        struct ifreq ifr;
        int ret;

        printf("barrier_server.\n");
        
        memset(&server, '0', sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(LOCAL_IP);
        server.sin_port = htons(LOCAL_PORT);
        
        listen_sd = socket(AF_INET, SOCK_STREAM, 0);
        
        /* Bind socket to the barrier sender interface */
        memset(&ifr,'0', sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "em1");
        if((ret = setsockopt(listen_sd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr))) < 0) {
                fprintf(stderr, "Error in setsockopt() for SO_BINDTODEVICE %s\n", strerror(errno));
                close(listen_sd);
                exit(-1);
        }
        
        bind(listen_sd, (struct sockaddr*) &server, sizeof(server));
        
        listen(listen_sd, 10);

        while(1) {
                struct sockaddr_in client;
                int sd;
                unsigned int len;
                long msg_buffer[2];

                len = sizeof(client);
                sd = accept(listen_sd, (struct sockaddr*) &client, &len);
                conn_sd = sd;
                
                recv(conn_sd, msg_buffer, MSGSIZE*2, 0);
                printf("barrier message received: %ld %ld\n", msg_buffer[0], msg_buffer[1]);
        }

        return NULL;
}

static int setup_ring_flows() {
        struct ifreq ifr;
        int sd;
        int ret = 0;
        struct sockaddr_in _server;

        /* Create the socket */
        sd = socket(AF_INET, SOCK_STREAM, 0);
        if((ret = sd) < 0) {
                fprintf(stderr, "Error in socket() creation: %s\n", strerror(errno));
                goto error;
        }

        printf("socket created.\n");
        
        /* Bind socket to the barrier sender interface */
        memset(&ifr,'0', sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "em2");
        if((ret = setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr))) < 0) {
                fprintf(stderr, "Error in setsockopt() for SO_BINDTODEVICE %s\n", strerror(errno));
                close(sd);
                goto error;
        }
        printf("em2 set\n");
    
        memset(&_server, '0', sizeof(_server));
        _server.sin_family = AF_INET;
        _server.sin_addr.s_addr = inet_addr(LOCAL_IP);
        _server.sin_port = htons(LOCAL_PORT);
        /* Connect with the barrier receiver */
        if((ret = connect(sd, (struct sockaddr *) &_server, sizeof(_server))) < 0) {
                fprintf(stderr, "Error in connect() %s\n", strerror(errno));
                close(sd);
                goto error;
        }

        printf("connected.\n");
        
        client_sd = sd;

error:
        return ret;
}

int main(int argc, char *argv[]) {
        int ret;
        
        ret = init_barrier();
        if(ret == 0) {
               ret = send_barrier_msg();
                printf("send ret = %d\n", ret);
        }
//        wait_barrier();

        sleep(2);
        destroy_barrier();

        return 0;
}
