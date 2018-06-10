#include <stdio.h> 
#include <string.h>    
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <signal.h>

#include "helper.h"

#define HB_SERVER_ADDR "10.0.0.11"
#define HB_SERVER_PORT 6789
#define MSGSIZE sizeof(long)

long base_time = 0;
long timeout_interval = 1000;

static int hb_fd;
static struct sockaddr_in hb_server;

static pthread_t tid;
static void *run_hb_loop(void *arg);

int init_sender() {
        struct sockaddr_in remote;
        unsigned int len;
        int ret = 0;
        int count;
        long request_msg[2];
        long msg_buffer[2]; 

        memset(&hb_server, '0', sizeof(hb_server));
        hb_server.sin_addr.s_addr = inet_addr(HB_SERVER_ADDR);
        hb_server.sin_family = AF_INET;
        hb_server.sin_port = htons(HB_SERVER_PORT);
        
        hb_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        
        /* Send request message [flag=0, whatever]. */
        request_msg[0] = 0; 
        request_msg[1] = 0;

        count = sendto(hb_fd, request_msg, MSGSIZE*2, 0, (struct sockaddr *) &hb_server, 
                    sizeof(hb_server));
        
        if (count != MSGSIZE*2) {
                perror("request sendto");
                ret = -1;
                goto error;
        }
       
        printf("NHeartbeat sender: request message [flag=0, 0] has been sent.\n");
        
        /* Receive base_time and timeout_interval. */
        memset(&remote, '0', sizeof(remote));
        len = sizeof(remote);
        
        if (recvfrom(hb_fd, msg_buffer, MSGSIZE, 0, (struct sockaddr *) &remote, &len) != MSGSIZE) {
                perror("recvfrom");
                ret = -1;
                goto error;
        }
        
        /* Get parameters. */
        base_time = msg_buffer[0];
        
        if (base_time <= 0) {
                fprintf(stderr, "Error values of base_time.\n");
                ret = -1;
                goto error;
        }
        
        printf("Reply message [base_time=%ld] is received.\n", base_time);

        /* Start sending thread. */
        pthread_create(&tid, NULL, run_hb_loop, NULL);
        
        printf("NHeartbeat sender has been initialized successfully.\n");

error:
        return ret;
}

/* Destroy. */
void destroy_sender() {
        pthread_cancel(tid);
        pthread_join(tid, NULL);
        
        printf("NHeartbeat sender has been destroyed.\n");
}

static void cleanup(void *arg) {
        printf("Clean up.\n");
        close(hb_fd);
}

/* Loop of heartbeat sending. */
static void *run_hb_loop(void *arg) {
        int count;
        long hb_msg[2]; /* [flag=1, epoch_id] */
        long current_epoch, sending_epoch;
        long diff_time;
        long timeout;
        //int count_tmp = 0;
        
        pthread_cleanup_push(cleanup, NULL);

        /* Set flag to indicate this is heartbeat. */
        hb_msg[0] = 1; 
        
        while(1) {     
                current_epoch = time_to_epoch(now_time());
                timeout = epoch_to_time(current_epoch);

                /* Sleep until the next epoch begins. */
                while ((diff_time = timeout - now_time()) > 0) {
                        struct timespec ts = time_to_timespec(diff_time);
                        nanosleep(&ts, NULL);
                }

                sending_epoch = time_to_epoch(now_time());
                hb_msg[1] = sending_epoch;

                count = sendto(hb_fd, &hb_msg, MSGSIZE*2, 0, (struct sockaddr *) &hb_server, 
                        sizeof(hb_server));

                if (count != MSGSIZE*2) {
                        perror("heartbeat sendto");
                        //break;
                }
                
                printf("NHeartbeat message [flag=1, epoch=%ld] has been sent.\n", hb_msg[1]);
        }

        pthread_cleanup_pop(1);
        
        return NULL;
}

static void sig_handler(int signo) {
        destroy_sender();
        printf("NHeartbeat sender ternimates in sig_handler.\n");
        exit(0);
}

int main(int argc, char *argv[]) {
        signal(SIGINT, sig_handler);
        signal(SIGTERM, sig_handler);
        signal(SIGKILL, sig_handler);
    
        if (init_sender() == 0) {
                while(1) {
                        sleep(1);
                }
         }
        
        destroy_sender();

        return 0;
}
