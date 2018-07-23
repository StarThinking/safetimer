#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "hb_common.h"

#include "st_receiver_api.h"

static pthread_t server_tid; /* Heartbeat server pthread id. */
static int server_fd;

static void *server_routine(void *arg);

int init_server() {
        int ret = 0;
        struct sockaddr_in server;
    
        
        memset(&server, '0', sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(ST_RECV_SERVER_ADDR);
        server.sin_port = htons(ST_RECV_SERVER_PORT);

        server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        bind(server_fd, (struct sockaddr*) &server, sizeof(server));

        /* Start heartbeat server thread. */
        pthread_create(&server_tid, NULL, server_routine, NULL);
    
        printf("init receiver server.\n");
    
        return ret;
}

/*
 * Entry function of state server.
 * Messages consist of two parts: [app_id, node ip].
 * If not stale, returns 0; if stale, returns 1.
 */
static void *server_routine(void *arg) {
        long msg_buffer[ST_RECEIVER_REQUEST_MSGSIZE/sizeof(long)];
        long app_id, start;
        unsigned int node_ip_bytes;
        struct in_addr ip_addr;
        struct sockaddr_in client;
        unsigned int len = sizeof(client);

        while (1) {
                if ((recvfrom(server_fd, &msg_buffer, ST_RECEIVER_REQUEST_MSGSIZE, 0, \
                        (struct sockaddr *) &client, &len)) != ST_RECEIVER_REQUEST_MSGSIZE) {
                        perror("recvfrom");
                        break;
                }

                app_id = msg_buffer[0];
                node_ip_bytes = (unsigned int) (ntohl(msg_buffer[1]));
                ip_addr.s_addr = node_ip_bytes;
                start = msg_buffer[2];

                printf("st receiver server received request:, app_id = %ld, node_ip_bytes = %u,"
                        " node_ip = %s, start = %ld\n", app_id, node_ip_bytes, inet_ntoa(ip_addr), start);

                if (app_id >= 0) {
                        long reply[1];

                        reply[0] = safetimer_check(app_id, inet_ntoa(ip_addr), start);

                        if(sendto(server_fd, reply, ST_RECEIVER_REPLY_MSGSIZE, 0, (struct sockaddr *) &client,
                                        sizeof(client)) != ST_RECEIVER_REPLY_MSGSIZE) {
                                perror("send reply");
                                break;
                        }

                        printf("replied result of safetimer_check = %ld to app %ld\n",
                                reply[0], app_id);
                }
        }

        return NULL;
}
 
static void sig_handler(int signo) {
	destroy_st_receiver_api();
        
	pthread_cancel(server_tid);
	pthread_join(server_tid, NULL);
        close(server_fd);

        printf("receiver server is destroyed.\n");
        exit(0);
}

int main(int argc, char *argv[]) {
        signal(SIGINT, sig_handler);
        signal(SIGTERM, sig_handler);
        signal(SIGKILL, sig_handler);
        
	init_server();
        init_st_receiver_api();
       
        while(1) {
                 sleep(1);
        }   
        
        return 0;
}
