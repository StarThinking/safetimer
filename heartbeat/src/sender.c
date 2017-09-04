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

#include "hb_config.h"
#include "helper.h"

#include "sender.h"

static int hb_fd;
static struct sockaddr_in hb_server;

extern long base_time;
long base_time = 0;
extern long timeout_interval;
long timeout_interval = 0;

static pthread_t tid;
static void *run_hb_loop(void *arg);
    
/*static void sig_handler(int signo) {
        if (signo == SIGINT) {
                destroy_sender(); 
                printf("Heartbeat sender terminates in sig_handler.\n");
                exit(0);
        }
}*/

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

        count = sendto(hb_fd, &request_msg, MSGSIZE*2, 0, (struct sockaddr *) &hb_server, 
                    sizeof(hb_server));
        
        if (count != MSGSIZE*2) {
                perror("request sendto");
                ret = -1;
                goto error;
        }
       
        printf("Heartbeat sender: request message [flag=0, 0] has been sent.\n");
        
        /* Receive base_time and timeout_interval. */
        memset(&remote, '0', sizeof(remote));
        len = sizeof(remote);
        
        if (recvfrom(hb_fd, &msg_buffer, MSGSIZE*2, 0, (struct sockaddr *) &remote, &len) != MSGSIZE*2) {
                perror("recvfrom");
                ret = -1;
                goto error;
        }

        /* Save parameters. */
        base_time = msg_buffer[0];
        timeout_interval = msg_buffer[1];

        if (base_time <= 0 || timeout_interval <=0) {
                fprintf(stderr, "Error values of base_time or timeout_interval.\n");
                ret = -1;
                goto error;
        }
        
        printf("Reply message [base_time=%ld, timeout_interval=%ld] is received.\n",
                base_time, timeout_interval);

        /* Start sending thread. */
        pthread_create(&tid, NULL, run_hb_loop, NULL);
        
        printf("Heartbeat sender has been initialized successfully.\n");

error:
        return ret;
}

/* Destroy. */
void destroy_sender() {
        pthread_cancel(tid);
        pthread_join(tid, NULL);
        printf("Heartbeat sender has been destroyed.\n");
}

static void cleanup(void *arg) {
        printf("Clean up.\n");
        close(hb_fd);
}

/* Loop of heartbeat sending. */
static void *run_hb_loop(void *arg) {
        int count;
        long hb_msg[2]; /* [flag=1, epoch_id] */
        long epoch;
        long diff_time;
        long timeout;
        
        pthread_cleanup_push(cleanup, NULL);

        /* Set flag to indicate this is heartbeat. */
        hb_msg[0] = 1; 
        
        while(1) {     
                epoch = time_to_epoch(now_time());
                timeout = epoch_to_time(epoch);

                /* Sleep until the next epoch begins. */
                if ((diff_time = timeout - now_time()) > 0) {
                        struct timespec ts = time_to_timespec(diff_time);
                        nanosleep(&ts, NULL);
                }

                hb_msg[1] = time_to_epoch(now_time()); 
        
                count = sendto(hb_fd, &hb_msg, MSGSIZE*2, 0, (struct sockaddr *) &hb_server, 
                        sizeof(hb_server));

                if (count != MSGSIZE*2) {
                        perror("request sendto");
                        break;
                }
                
                printf("Heartbeat message [flag=1, epoch=%ld] has been sent.\n", hb_msg[1]);
        }

        pthread_cleanup_pop(1);
        
        return NULL;
}
        /*

        printf("packet received, ret = %d, base_time = %ld, timeout_interval = %ld.\n", 
                ret, recv_buf[0], recv_buf[1]);

        fp = fopen("/sys/kernel/debug/hb_sender_tracker/base_time", "r+");
        if(fp == NULL)
                printf("failed to open file!\n");
        else {
                buf = (char*) calloc(20, sizeof(char));
                sprintf(buf, "%ld", recv_buf[0]);
                fputs(buf, fp);
                free(buf);
        }
        fclose(fp);
        
        fp = fopen("/sys/kernel/debug/hb_sender_tracker/timeout_interval", "r+");
        if(fp == NULL)
                printf("failed to open file!\n");
        else {
                buf = (char*) calloc(20, sizeof(char));
                sprintf(buf, "%ld", recv_buf[1]);
                fputs(buf, fp);
                free(buf);
        }
        fclose(fp);

        timeout_interval = recv_buf[1];

        long begin_t = now();
        while(1) {  
                long hb_send_time = now();
                
                // update hb_send_time as hb_send_compl_time for the first hb send
                if(first_hb_send) {
                        fp = fopen("/sys/kernel/debug/hb_sender_tracker/hb_send_compl_time", "r+");
                        if(fp == NULL)
                                printf("failed to open file!\n");
                        else {
                                buf = (char*) calloc(20, sizeof(char));
                                sprintf(buf, "%ld", hb_send_time);
                                fputs(buf, fp);
                                free(buf);
                        }
                        fclose(fp);
                        first_hb_send = 0;
                        printf("update hb_send_compl_time as now hb_send_time %ld for the first hb send\n",
                                hb_send_time);
                }
        }
        
        long end_t = now();
        printf("Time used to send 10000 packets is %ld\n", end_t - begin_t);
  */
