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

#include "hb_common.h"
#include "st_sender_api.h"

static pthread_t server_tid; 
static int server_fd;

static void *server_routine(void *arg);
static void cleanup(void *arg);

#define TMP_APP_ID 1

static int init_server() {
        int ret = 0;
        struct sockaddr_in server;
        
        memset(&server, '0', sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(ST_SEND_SERVER_ADDR);
        server.sin_port = htons(ST_SEND_SERVER_PORT);

        server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        bind(server_fd, (struct sockaddr*) &server, sizeof(server));

        pthread_create(&server_tid, NULL, server_routine, NULL);
        printf("sender server thread started.\n");
       
        return ret;
}

static void cleanup(void *arg) {  
        printf("clean up request server.\n");
        close(server_fd);
}

static void *server_routine(void *arg) {
        long msg_buffer[ST_SENDER_REQUEST_MSGSIZE/sizeof(long)];
        struct sockaddr_in client;
        unsigned int len = sizeof(client);

        pthread_cleanup_push(cleanup, NULL);

        while (1) {
                long request_type, timeout, node_id;
                long reply = 0;

                if((recvfrom(server_fd, msg_buffer, ST_SENDER_REQUEST_MSGSIZE, 0, \
                                (struct sockaddr *) &client, &len)) != ST_SENDER_REQUEST_MSGSIZE) {
                        perror("recvfrom");
                        break;
                }

                request_type = msg_buffer[0];
                timeout = msg_buffer[1];
                node_id = msg_buffer[2];
                printf("st_sender receive request: request_type = %ld, timeout/valid_time = %ld, " 
                        "node_id = %ld\n", request_type, timeout, node_id);
                
                if (request_type == 0) { 
                        reply = safetimer_send_heartbeat(TMP_APP_ID, timeout, node_id);
                        printf("safetimer_send_heartbeat() returns %ld\n", reply);
                } else if (request_type == 1) { 
                        safetimer_update_valid_time(timeout);
                } else {
			printf("Error: wrong request_type %ld received!\n", request_type);
		}

                // send reply
                if(sendto(server_fd, &reply, ST_SENDER_REPLY_MSGSIZE, 0, (struct sockaddr *) &client, \
                            sizeof(client)) != ST_SENDER_REPLY_MSGSIZE) {
                        perror("send reply");
                        break;
                } 
                printf("reply = %ld has been sent\n", reply);
        }

        pthread_cleanup_pop(1);
        return NULL;
}

static void sig_handler(int signo) {
        destroy_st_sender_api();

        pthread_cancel(server_tid);
        pthread_join(server_tid, NULL);
        close(server_fd);
        
        printf("sender server is destroyed.\n");
        exit(0);
}

int main(int argc, char *argv[]) {
        signal(SIGINT, sig_handler);
        signal(SIGTERM, sig_handler);
        signal(SIGKILL, sig_handler);

        init_server();
        init_st_sender_api();

        while (1) {
                printf("safetimer sender main thread sleeps 1s.\n");
                sleep(1);
        };  

        return 0;
}
