#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <semaphore.h>
#include "queue.h"

#define LOCALIP "10.0.0.12"
#define REMOTEIP "10.0.0.14"
#define HBSENDERIP "10.0.0.13"
#define MSGSIZE sizeof(long)
//#define MSGSIZE 8

//#define LOCAL
//#define REMOTE

static long timeout_intvl_ms = 0;
//static bool expired = true;
struct queue *queue;
static int expired_num = 0;
static int local_sendself_sock;
static int remote_sendself_sock;
static int hb_sender_sock;

static pthread_mutex_t lock;
static sem_t sem;
static pthread_t expirator_tid;
static pthread_t receiver_tids[5];
static int connfds[5];
static int listenfd = 0, connfd = 0;
static int connfd_num = 0, receiver_num = 0;

void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("SIGINT handled and expired_num = %d\n", expired_num);
        int i;
        for(i=0; i<connfd_num; i++)
            close(connfds[i]);
        close(listenfd);
        for(i=0; i<receiver_num; i++)
            pthread_cancel(receiver_tids[i]);
        pthread_cancel(expirator_tid);
        exit(0);
    }
}

long now() {   
    struct timespec spec;    
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec/1.0e6;
}

long first_timeout(long now_ms) {   
    return now_ms + timeout_intvl_ms;
}

struct timespec sleep_time(long sleep_t) {
    struct timespec sleep_ts;
    sleep_ts.tv_sec = sleep_t / 1000;
    sleep_ts.tv_nsec = (sleep_t % 1000) * 1.0e6;
    return sleep_ts;
}

// thread for checking expiration
void *expirator(void *arg) {
    int sock = *(int *) arg;
    printf("[expirator] expirator thread created, local client sock = %d\n", sock);
    long timeout_t, begin_t, end_t, process_t;
    struct timespec sleep_ts;
    timeout_t = first_timeout(now());
    enqueue(queue, timeout_t, true);
    char *buf = malloc(MSGSIZE);
    
    while(1) {
        while(now() < timeout_t) {
            sleep_ts = sleep_time(timeout_t - now());
            nanosleep(&sleep_ts, NULL);
        }
        timeout_t += timeout_intvl_ms;

       // begin_t = now();
#if defined(LOCAL) || defined(REMOTE)
        // send a packet to self
        int ret = send(sock, buf, MSGSIZE, 0);
        if(ret != MSGSIZE) {
            printf("[expirator] send incorrectly, ret = %d\n", ret);
            break;
        }
        // wait for semaphore
        printf("[expirator] wait for sem post by receiver\n");
        sem_wait(&sem); 
#endif
        //end_t = now();
        //process_t = end_t - begin_t;
       // printf("[expirator] time to check expiration, waiting send-self time = %ld ms\n", process_t);
        // extend timeout with waiting send-self time for compensation
       // timeout_t += process_t;
        
        // do expiration check
        pthread_mutex_lock(&lock);
       
        struct qnode *node;
        while((node = dequeue(queue)) != NULL) {
            long expire_time = node->timestamp;
            bool expired = node->expired;
            if(expired == true) {
                printf("\n[expirator] !!! expired at expiration time %ld\n\n", expire_time);
                expired_num ++;
            } else {
                printf("[expirator] not expired at expiration time %ld\n", expire_time);
            }
            free(node);
        }

        enqueue(queue, timeout_t, true);
        
        pthread_mutex_unlock(&lock);
    }
}

// thread for receiving heartbeat
void *receiver(void *arg) {
    //char *buf = malloc(MSGSIZE);
    long now_t;
    int connfd = *(int *) arg;
    bool first_hb_received = false;
    int ret;
    while(1) {
	int ret = recv(connfd, &now_t, MSGSIZE, 0);
	//int ret = recv(connfd, buf, MSGSIZE, 0);
	if(ret <= 0) {
	    close(connfd);
	    break;
	}
	if(ret != MSGSIZE) printf("warning recv ret=%d\n", ret);
            
        printf("[receiver] received\n");

        if(connfd == hb_sender_sock) { // packets from heartbeat sender
            printf("[receiver] receievd heartbeat from %s, timestamp = %ld\n", HBSENDERIP, now_t);
            //printf("[receiver] receievd heartbeat from %s\n", HBSENDERIP);
            pthread_mutex_lock(&lock);
            // record expired_num after first receiving hb
            if(!first_hb_received) {
                expired_num = 0;
                first_hb_received = true;
            }
            //expired = false;
            ret = update_queue(queue, now_t);
            if(ret < 0) 
                printf("update_queue wrong with ret = %d\n", ret);
            pthread_mutex_unlock(&lock);
        }
#if defined(LOCAL)
        else if(connfd == local_sendself_sock) { // packets from localhost
            printf("[receiver] received local send-self from %s and wake up expirator\n", LOCALIP);
            sem_post(&sem);
        }
#elif defined(REMOTE)
        else if(connfd == remote_sendself_sock) { // packets from remote_sendself_sock
            printf("[receiver] received remote send-self from %s and wake up expirator\n", REMOTEIP);
            sem_post(&sem);
        }
#endif
    } 
}

int main(int argc, char *argv[]) {

    signal(SIGINT, sig_handler);

    if(argc != 2) {
        printf("./receiver [timeout ms]\n");
	return 1;
    }

    queue = create_queue();

    timeout_intvl_ms = atoi(argv[1]);
    printf("timeout = %ld ms\n", timeout_intvl_ms);
    sem_init(&sem, 0, 0); 
    
    struct sockaddr_in serv_addr, remote_addr; 

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    //serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_addr.s_addr = inet_addr(LOCALIP);
    serv_addr.sin_port = htons(5001); 
    
/*    char *optval2 = "em1";
    if(setsockopt(listenfd, SOL_SOCKET, SO_BINDTODEVICE, optval2, 3) < 0)
    {
        printf("listenfd setsockopt failed\n");
        close(listenfd);
        exit(2);
    }
*/    
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(listenfd, 10); 
    
   /* remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(REMOTEIP);
    remote_addr.sin_port = htons(5001); 

    int sock = 0;
#if defined(LOCAL)
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect failed. Error");
        return 1;
    } else {
        printf("[main] local send-self sock connected\n");
    }
#elif defined(REMOTE)
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(connect(sock, (struct sockaddr *) &remote_addr, sizeof(remote_addr)) < 0) {
        perror("connect failed. Error");
        return 1;
    } else {
        printf("[main] remote send-self sock connected\n");
    }
#endif
    // create expirator thread
    pthread_create(&expirator_tid, NULL, expirator, &sock);
*/
    while(1) {
        struct sockaddr_in client;
        unsigned int client_len = sizeof(client);
        connfd = accept(listenfd, (struct sockaddr*) &client, &client_len);
    
/*    char *optval2 = "em1";
    if(setsockopt(connfd, SOL_SOCKET, SO_BINDTODEVICE, optval2, 3) < 0)
    {
        printf("connfd setsockopt failed\n");
        close(connfd);
        exit(2);
        connfds[connfd_num++] = connfd;
    }*/

        printf("[main] client connection accepted, sock = %d, ip = %s\n", connfd, inet_ntoa(client.sin_addr));
        
        // determine sockfd-ip mapping
        if(strcmp(inet_ntoa(client.sin_addr), LOCALIP) == 0) 
            local_sendself_sock = connfd;
        
        if(strcmp(inet_ntoa(client.sin_addr), HBSENDERIP) == 0) 
            hb_sender_sock = connfd;
        
        if(strcmp(inet_ntoa(client.sin_addr), REMOTEIP) == 0) 
            remote_sendself_sock = connfd;

	int *tmp = malloc(sizeof(int));
	*tmp = connfd;
	pthread_t receiver_tid;
	pthread_create(&receiver_tid, NULL, receiver, tmp);
        receiver_tids[receiver_num++] = receiver_tid;
    }
}
