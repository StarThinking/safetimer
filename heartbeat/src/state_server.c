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
#include "state_server.h"

static hash_table app_nodes_ht;

static pthread_t server_tid; /* Heartbeat server pthread id. */
static int server_fd;

static void *state_server(void *arg);
static void cleanup(void *arg);

static long get_state(long app_id, char *node);

static long get_state(long app_id, char *node_ip) {
        size_t value_size;
        hash_table *node_states_ht;
        long *node_state;

        node_states_ht = (hash_table *) ht_get(&app_nodes_ht, &app_id, sizeof(long), &value_size);
        if (node_states_ht != NULL) {
                node_state = (long *) ht_get(node_states_ht, node_ip, strlen(node_ip), &value_size);
                if (node_state != NULL) {
                        printf("The state of node %s for app %ld is %ld.\n",
                                    node_ip, app_id, *node_state);
                } else {
                        printf("error: get_state can't find entry for app_id %ld node_ip %s\n", 
                                app_id, node_ip);
                        return 0;
                }
        } else {
                printf("error: get_state can't find for app_id %ld\n", app_id);
                return 0;
        }

        return *node_state;
}

void put_state(long app_id, char *node_ip, long state) {
        size_t value_size;
        hash_table *node_states_ht;
        long *node_state_p;
        long *tmp;

        node_states_ht = (hash_table *) ht_get(&app_nodes_ht, &app_id, sizeof(long), &value_size);
        if (node_states_ht == NULL) {
                node_states_ht = (hash_table *) calloc(sizeof(hash_table), 1);
                ht_init(node_states_ht, HT_NONE, 0.05);
                ht_insert(&app_nodes_ht, &app_id, sizeof(long), node_states_ht, sizeof(hash_table));
                printf("State server: inserted new hashtable for app %ld.\n", app_id);
        }
        
        //printf("sizeof(*node_ip) = %lu\n", sizeof(*node_ip));

        node_state_p = (long *) ht_get(node_states_ht, node_ip, strlen(node_ip), &value_size);
        if (node_state_p == NULL) {
                node_state_p = (long *) calloc(sizeof(long), 1);
                ht_insert(node_states_ht, node_ip, strlen(node_ip), node_state_p, sizeof(long));
                printf("State server: inserted new entry for node %s\n", node_ip);
        }
        
        if (*node_state_p != state) {
                printf("State server: change the state of app %ld, node %s to %ld\n", 
                            app_id, node_ip, state);
                *node_state_p = state;
        }

        return;
}

int init_state_server() {
        int ret = 0;
        struct sockaddr_in server;
        
        ht_init(&app_nodes_ht, HT_NONE, 0.05);  

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

        ht_destroy(&app_nodes_ht);
}

/*
 * Entry function of state server.
 * Messages consist of two parts: [app_id, node ip].
 * If not stale, returns 0; if stale, returns 1.
 */
static void *state_server(void *arg) {
        long msg_buffer[2];
        long app_id;
        unsigned int node_ip_bytes;
        struct in_addr ip_addr;
        struct sockaddr_in client;
        unsigned int len = sizeof(client);

        pthread_cleanup_push(cleanup, NULL);

        while (1) {
                if((recvfrom(server_fd, &msg_buffer, MSGSIZE*2, 0, (struct sockaddr *) &client, &len)) != MSGSIZE*2) {
                        perror("recvfrom");
                        break;
                }

                app_id = msg_buffer[0];
                node_ip_bytes = (unsigned int) (ntohl(msg_buffer[1]));
                ip_addr.s_addr = node_ip_bytes;

                printf("State request received, app_id = %ld, node_ip_bytes = %u, node_ip = %s\n",
                            app_id, node_ip_bytes, inet_ntoa(ip_addr));

                //get_state(app_id, inet_ntoa(ip_addr));

                if (app_id >= 0) {
                        /* For requests, reply the state of that app. */
                        long reply[1];

                        reply[0] = get_state(app_id, inet_ntoa(ip_addr));

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
