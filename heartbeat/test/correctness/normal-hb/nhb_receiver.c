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
#include <semaphore.h>

#include "hashtable.h"
#include "list.h"

#include "helper.h"

#define HB_SERVER_ADDR "10.0.0.11"
#define HB_SERVER_PORT 6789
#define MSGSIZE sizeof(long)

long base_time;
long timeout_interval = 1000;

static hash_table epoch_list_ht;
static pthread_mutex_t epoch_list_ht_lock;

static pthread_t expire_checker_tid;
static void *expire_checker(void *arg);
static void expiration_check_for_epoch(long epoch);

static pthread_t server_tid; /* Heartbeat server pthread id. */
static int server_fd;

static int init_receiver();
static void *hb_server(void *arg);
static void destroy_receiver();
static void cleanup(void *arg);
static void cancel_receiver();
static void join_receiver();
static int ip_equal(void *a, void *b);
static void free_val(void *val);

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

        ht_init(&epoch_list_ht, HT_NONE, 0.05);

        /* Start heartbeat server thread. */
        pthread_create(&server_tid, NULL, hb_server, NULL);
        printf("NHeartbeat receiver thread started.\n");

        /* Start expiration checker thread. */
        pthread_create(&expire_checker_tid, NULL, expire_checker, NULL);
        printf("Expire checker thread started.\n");
       
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
        //int count = 0;

        pthread_cleanup_push(cleanup, NULL);

        while (1) {
          /*      if (10 == count++) {
                        struct timespec ts = time_to_timespec(2500);
                        printf("sleep 2.5 second.\n");
                        nanosleep(&ts, NULL);
                }
            */            
                if((recvfrom(server_fd, &msg_buffer, MSGSIZE*2, 0, (struct sockaddr *) &client, &len)) != MSGSIZE*2) {
                        perror("recvfrom");
                        break;
                }
                
                flag = msg_buffer[0];

                if (flag == 0) {
                        /* For requests, reply [base_time]. */
                        long reply = base_time;

                        printf("NHeartbeat server: request from %s:%u.\n",
                                inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                        
                        if(sendto(server_fd, &reply, MSGSIZE, 0, (struct sockaddr *) &client,
                                    sizeof(client)) != MSGSIZE) {
                                perror("send reply");
                                break;
                         }
                } else if (flag == 1) {
                        /* Heartbeats. */
                        long epoch = -1;
                        list_t **ip_list;
                        size_t value_size;
                        char saddr[INET_ADDRSTRLEN];

                        memcpy(saddr, inet_ntoa(client.sin_addr), INET_ADDRSTRLEN);
                        epoch = msg_buffer[1];
                    
                        if (epoch < 0) {
                                fprintf(stderr, "Queue error: epoch < 0.\n");
                                break;
                        }

                        printf("NHeartbeat server: heartbeat from %s:%u for epoch %ld.\n",
                                inet_ntoa(client.sin_addr), ntohs(client.sin_port), epoch);
			
			pthread_mutex_lock(&epoch_list_ht_lock);

                	// Touch the node for the epoch
                	ip_list = (list_t**) ht_get(&epoch_list_ht, &epoch, sizeof(long), &value_size);
                	if (ip_list != NULL) {
                        	list_node_t *ip_to_remove = list_find(*ip_list, saddr);
                        	if (ip_to_remove != NULL) {
                                	list_remove(*ip_list, ip_to_remove);
                                	printf("NHeartbeat server: remove node %s from epoch list %ld.\n", saddr, epoch);
                       	 	}
                	}

                	// Insert Ip into the Ip list of the next epoch
               	 	epoch ++;

                	ip_list = (list_t**) ht_get(&epoch_list_ht, &epoch, sizeof(long), &value_size);
                	if (ip_list == NULL) {
                        	list_t *tmp = list_new();
                        	tmp->match = ip_equal;
                        	tmp->free = free_val;
                       	 	ip_list = &tmp;
                        	ht_insert(&epoch_list_ht, &epoch, sizeof(long), ip_list, sizeof(list_t*));
                        	//printf("Queue: create new node list for epoch %ld.\n", epoch);
                	}

                	if (list_find(*ip_list, saddr) == NULL) {
                        	// Allocate the memory of _saddr in heap
                        	char *_saddr = (char*) calloc(INET_ADDRSTRLEN, sizeof(char));
                        	strcpy(_saddr, saddr);
                        	list_rpush(*ip_list, list_node_new(_saddr));
                        	printf("NHeartbeat server: add node %s into the node list for epoch %ld.\n", saddr, epoch);
                	}

                	pthread_mutex_unlock(&epoch_list_ht_lock);
                }
        }

        pthread_cleanup_pop(1);
        return NULL;
}

/*
 * Thread for expiration check
 */
static void *expire_checker(void *arg) {
        long timeout;
        long epoch_to_check, epoch_now;
        long diff_time;

        while (1) {
                epoch_to_check = time_to_epoch(now_time());
                timeout = epoch_to_time(epoch_to_check);

                if ((diff_time = timeout - now_time()) > 0) {
                        struct timespec ts = time_to_timespec(diff_time);
                        nanosleep(&ts, NULL);
                }

                do {
                        expiration_check_for_epoch(epoch_to_check);
                        epoch_now = time_to_epoch(now_time());

                        if (epoch_to_check + 1 < epoch_now)
                                printf("Checker: more than 1 epoch should be checked!\n");

                        epoch_to_check ++;
                } while (epoch_to_check < epoch_now);
        }

        return NULL;
}

static void expiration_check_for_epoch(long epoch) {
        list_t **ip_list;
        size_t value_size;

	/* Before perfroming expiration check, grab the lock. */
        pthread_mutex_lock(&epoch_list_ht_lock);

        ip_list = (list_t**) ht_get(&epoch_list_ht, &epoch, sizeof(long), &value_size);
        if (ip_list != NULL) {
               list_iterator_t *it = list_iterator_new(*ip_list, LIST_HEAD);
               list_node_t *next = list_iterator_next(it);

               if (next == NULL)
                        printf("Checker: no timeout for epoch %ld.\n", epoch);

               while (next != NULL) {
                        printf("\n\tChecker: node %s timeout for epoch %ld !\n\n",
                                (char*) next->val, epoch);

                        list_remove(*ip_list, next);
                        next = list_iterator_next(it);
               }

               // Delete iterator, list and hashtable kv entry.
               list_iterator_destroy(it);
               list_destroy(*ip_list);
               ht_remove(&epoch_list_ht, &epoch, sizeof(long));
        }

        pthread_mutex_unlock(&epoch_list_ht_lock);
}

static void sig_handler(int signo) {
        destroy_receiver();
        printf("NHeartbeat receiver ternimates in sig_handler.\n");
        exit(0);
}

static void cancel_receiver() {
        pthread_cancel(server_tid);
        pthread_cancel(expire_checker_tid);
}

static void join_receiver() {
        pthread_join(server_tid, NULL);
        printf("NHeartbeat receiver thread joined.\n");

        pthread_join(expire_checker_tid, NULL);
        printf("Queue expire_checker thread joined.\n");
}

static void cleanup(void *arg) {  
        printf("Clean up NHeartbeat receiver.\n");
        close(server_fd);
        ht_destroy(&epoch_list_ht);
}

static void destroy_receiver() {
        cancel_receiver();
        join_receiver();
        printf("NHeartbeat receiver has been destroyed.\n");
}

static int ip_equal(void *a, void *b) {
        return 0 == strcmp((char*)a, (char*)b);
}
    
static void free_val(void *val) {
        free(val);
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
