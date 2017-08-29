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
#include "barrier.h"

#define BARRIER_SERVER_IF "em1"
#define BARRIER_CLIENT_IF "em2"

/* Barrier server*/
static pthread_t server_tid; /* Barrier server pthread id */
static fd_set active_fd_set, read_fd_set;
static int listen_fd;
static int conn_fds[IRQ_NUM];

static void *barrier_server(void *arg);
static int validate_connection(int client_port, long *rx_queue);
static int get_sport_by_rx_queue(const int rx_queue);
static char *concat(const char *s1, const char *s2);

/* Barrier client*/
static int client_fds[IRQ_NUM]; /* Barrier client socket fds */

static int setup_rx_queue_flows();
static int clear_sport_on_rx_queues();

/*
 * 1. Start barrier server and configure em1 as the interface for receiving.
 * 2. Set up rx queue flows from em2 to all the rx queues on em1
 *    and configure em2 as the interface for sending.
 */
int init_barrier() {
        struct sockaddr_in server;
        struct ifreq ifr;
        int ret = 0;

        memset(&server, '0', sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(LOCAL_IP);
        server.sin_port = htons(LOCAL_PORT);
        
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        
        /* Set listen socket to interface em1 where barrier messages will be received. */
        memset(&ifr,'0', sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), BARRIER_SERVER_IF);
        if ((ret = setsockopt(listen_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr))) < 0) {
                perror("setsockopt");
                close(listen_fd);
                goto error;
        }
      
        /* Bind listen socket. */
        bind(listen_fd, (struct sockaddr*) &server, sizeof(server)); 
        listen(listen_fd, 10);

        /* Start barrier server. */
        pthread_create(&server_tid, NULL, barrier_server, NULL);
        printf("Barrier server thread started.\n");
        
        sleep(1);

        /* Set up rx queue flows from em2 to em1 via all the rx queues on em1. */
        if ((ret = setup_rx_queue_flows()) < 0) {
                fprintf(stderr, "Error: setup_rx_queue_flows() failed with ret %d.\n", ret);
                goto error;
        } else {
                printf("It is successful to set up %u rx queue flows from %s on %s --> %s on %s.\n",
                        IRQ_NUM, SELF_IP, BARRIER_CLIENT_IF, LOCAL_IP, BARRIER_SERVER_IF);
        }
        
        printf("Barrier server and rx queue flows have been initialized successfully.\n");

error:
        return ret;
}

/*
 * Clear up all the resources used for barrier client and server.
 */
void destroy_barrier() {
        int i;

        /* First, cancel barrier server thread. */
        pthread_cancel(server_tid);
        pthread_join(server_tid, NULL);
        
        /* Second, close all socket fds. */
        FD_ZERO(&active_fd_set);
        close(listen_fd);
        for (i=0; i<IRQ_NUM; i++) {
                close(client_fds[i]);
                close(conn_fds[i]);
        }

        printf("Barrier server and clients have been destroyed.\n");
}

/*
 * Send barrier message via all connected rx queue flows from em2 to em1.
 * Barrier message consists of data of two longs:
 * 1.Epoch id;
 * 2.Index (0...IRQ_NUM) 
 */
int send_barrier_message(long epoch_id) {
        int ret = 0;
        int i;
        int msg_size = 2*MSGSIZE;
        
        for (i=0; i<IRQ_NUM; i++) {
                long message[2];
                message[0] = epoch_id;
                message[1] = i;
                if (send(client_fds[i], message, msg_size, 0) != msg_size) {
                        perror("send");
                        ret = -1;
                        goto error;
                }
        }

        printf("Barrier client: barrier messages for epoch_id %ld "
                "have been sent out along all the flows.\n", epoch_id);

error:
        return ret;
}

/*
 * Entry function of barrier server.
 */
static void *barrier_server(void *arg) {
        struct sockaddr_in client;
        unsigned int len;
        long msg_buffer[2];
        int fd;
        int i;
        long rx_queue;

        FD_ZERO(&active_fd_set);
        FD_SET(listen_fd, &active_fd_set);
 
        while (1) {
                /* Block until input arrives on one or more active sockets. */
                read_fd_set = active_fd_set;
                if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
                        perror ("select");
                        break;
                }

                /* Service all the sockets with input pending. */
                for (i = 0; i < FD_SETSIZE; ++i) {
                        if (FD_ISSET(i, &read_fd_set)) {
                                if (i == listen_fd) { 
                                        /* New connection request on listen socket. */
                                        len = sizeof(client);
                                        fd = accept(listen_fd, (struct sockaddr*) &client, &len);
                                        if (fd < 0) {
                                                perror("accept");
                                                break;
                                        }

                                        /* 
                                         * If valid, reply client a message contains positive values (rx_queue);
                                         * Othersie, reply -1 so that client will close connection itself.
                                         */
                                        if (validate_connection((int)htons(client.sin_port), &rx_queue)) {
                                                send(fd, &rx_queue, MSGSIZE, 0);
                                                conn_fds[rx_queue - IRQ_NUM] = fd;
                                                
                                                /* Add this conn fd into select fd set.*/
                                                FD_SET(fd, &active_fd_set);
                                                
                                                printf("Barrier server: connection from host:port %s:%u " 
                                                        "via rx queue %ld is valid.\n",
                                                        inet_ntoa(client.sin_addr), ntohs(client.sin_port), rx_queue);
                                        } else {
                                               send(fd, &rx_queue, MSGSIZE, 0);
                                               close(fd); 
                                               
                                               printf("Barrier server: connection from host:port %s:%u " 
                                                        "via rx queue %ld is redundant.\n",
                                                        inet_ntoa(client.sin_addr), ntohs(client.sin_port), rx_queue);
                                        }
                                } else {
                                        /* Data arriving on an already-connected socket. */
                                        recv(i, msg_buffer, MSGSIZE*2, 0);
                                        printf("Barrier message %ld %ld received from host:port %s:%u\n",
                                                msg_buffer[0], msg_buffer[1],
                                                inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                                        /*if(strcmp(inet_ntoa(client.sin_addr), SELF_IP) == 0) {
                                                
                                        } else {
                                        }*/
                                }
                        }
                }
        }

        return NULL;
}

/*
 * Check if this connection is valid.
 * Validation here means the rx queue where this connection
 * passes through has NOT been used by previous connections yet.
 * If valid, return 1 and assign rx_queue with the rx queue id; 
 * Otherwise, return 0 and set rx_queue to -1.
 * If error happens, return -1.
 */
static int validate_connection(int client_port, long *rx_queue) {
        int i;
        int sport_of_rx_queue;
        *rx_queue = -1;

        /* 
         * Iterate to check the source port for each rx quueue.
         * If two ports match, it means this new connection is valid
         * because the rx queue has been occupied by previous connections.
         */
        for (i = BASE_IRQ; i < BASE_IRQ + IRQ_NUM; i++) {
                /* Error */
                if ((sport_of_rx_queue = get_sport_by_rx_queue(i)) < 0) {
                        fprintf(stderr, "Error happens when getting sport by rx queue.\n");
                        return -1;
                }

                /* Valid */
                if (client_port == sport_of_rx_queue) {
                        *rx_queue = i;
                        return 1;
                }
        }
        
        /* Invalid */
        return 0;
}

/*
 * Read the source port of the specified rx queue from debugfs.
 * If successful, return positive sport; otherwire, return -1.
 */
static int get_sport_by_rx_queue(const int rx_queue) {
        char *r1, *r2, *r3;
        char rx_queue_str[4];
        FILE *fd;
        char buf[8];
        int port = -1;

        sprintf(rx_queue_str, "%d", rx_queue);
        r1 = concat("/sys/kernel/debug/", LOCAL_IP);
        r2 = concat(r1, "/");
        r3 = concat(r2, rx_queue_str);

        if ((fd = fopen(r3, "r")) == NULL) {
                perror("fopen");
                goto error;
        }
        
        fscanf(fd, "%s", buf);
        port = atoi(buf);
    
error:
        fclose(fd);
        free(r1);
        free(r2);
        free(r3);

        return port;
}

/*
 * Utility function.
 */
static char *concat(const char *s1, const char *s2) {
        char *result = malloc(strlen(s1) + strlen(s2) + 1);
        strcpy(result, s1);
        strcat(result, s2);

        return result;
}

/*
 * Set up all the flows from em2 to rx queues on em1
 */
static int setup_rx_queue_flows() {
        struct sockaddr_in server;
        struct ifreq ifr;
        int fd;
        int ret = 0;
        int count = 0;
        long rx_queue = -1;

        if ((ret = clear_sport_on_rx_queues()) < 0) {
                fprintf(stderr, "Error: failed to clear up source ports on rx queues.\n");
                goto error;
        }

        memset(&server, '0', sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(LOCAL_IP);
        server.sin_port = htons(LOCAL_PORT);
        
        while (count < 4) {        
                /* Create the socket. */
                fd = socket(AF_INET, SOCK_STREAM, 0);
                if ((ret = fd) < 0) {
                        perror("socket");
                        goto error;
                }

                /* Set socket to interface BARRIER_CLIENT_IF where barrier messages will be sent. */
                memset(&ifr,'0', sizeof(ifr));
                snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), BARRIER_CLIENT_IF);
                if ((ret = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr))) < 0) {
                        perror("setsockopt");
                        close(fd);
                        goto error;
                }
    
                /* Connect to the barrier receiver. */
                if ((ret = connect(fd, (struct sockaddr *) &server, sizeof(server))) < 0) {
                        perror("connect");
                        close(fd);
                        goto error;
                }
          
                /* 
                 * Wait for response from barrier server. 
                 * If positive values received, connection is valid;
                 * Otherwise, the connection is redundant.
                 */
                recv(fd, &rx_queue, MSGSIZE, 0);
                if (rx_queue > 0) {
                        /* Save client socket fd. */
                        client_fds[rx_queue - BASE_IRQ] = fd;
                        count ++;
                } else {
                        close(fd);
                        sleep(1);
                }
        }

error:
        return ret;
}

static int clear_sport_on_rx_queues() {
        char *r1, *r2, *r3;
        FILE *fd;
        int ret = 0;

        r1 = concat("/sys/kernel/debug/", LOCAL_IP);
        r2 = concat(r1, "/");
        r3 = concat(r2, "clear");

        if ((fd = fopen(r3, "r+")) == NULL) {
                perror("fopen");
                ret = -1;
                goto error;
        }
        
        /* It doesn't matter what is put to clear debugfs. */
        fputs("1", fd);
     
error:
        fclose(fd);
        free(r1);
        free(r2);
        free(r3);

        return ret;
        
} 

int main(int argc, char *argv[]) {
        int ret;
        
        ret = init_barrier();
        if (ret < 0)
            return -1;
        
        ret = send_barrier_message(0);
        //sleep(1);

        ret = send_barrier_message(1);
        sleep(1);
        
        sleep(2);
        destroy_barrier();

        return 0;
}
