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

#include "hb_config.h"
#include "helper.h"

static int client_tcp_fd;
static int client_udp_fd;
struct sockaddr_in hb_server;

extern long base_time;
long base_time = 0;
extern long timeout_interval;
long timeout_interval = 0;

static int fetch_parameter();
static int init_sender_client();

int init_sender();
void destroy_sender();
void start_send_heartbeat();
    
static void sig_handler(int signo) {
      if (signo == SIGINT) {
            destroy_sender();
            exit(0);
      }
}

/* Initialize TCP temporary connection and UDP client socket. */
int init_sender() {
        int ret = 0;

        if ((ret = fetch_parameter()) < 0) {
                fprintf(stderr, "Heartbeat sender failed to fetch parameters.\n");
                goto error;
        }
        
        if ((ret = init_sender_client()) < 0) {
                fprintf(stderr, "Heartbeat sender failed to init heartbeat sender.\n");
                goto error;
        }

error:
        return ret;
}

/* Loop of heartbeat sending */
void start_send_heartbeat() {
        /* Sleep until the next epoch begins.*/
        long now_t = now_time();
        long epoch_id = time_to_epoch(now_t);

        sendto(client_udp_fd, &epoch_id, MSGSIZE, 0, (struct sockaddr *) &hb_server, sizeof(hb_server));
        printf("sent\n");
}

/* Destroy */
void destroy_sender() {
        close(client_udp_fd);
}

static int fetch_parameter() {
        struct sockaddr_in server;
        long request_msg[2];
        long msg_buffer[2];
        int fd;
        int ret = 0;
        
        /*  Connect to request server. */
        memset(&server, '0', sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(HB_SERVER_ADDR);
        server.sin_port = htons(REQUEST_SERVER_PORT);
        
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if ((ret = fd) < 0) {
                perror("socket");
                goto error;
        }

        if ((ret = connect(fd, (struct sockaddr *) &server, sizeof(server))) < 0) {
                perror("connect");
                close(fd);
                goto error;
        }
       
        /* Save TCP client fd. */
        client_tcp_fd = fd;

        printf("Heartbeat sender: connect to barrier server %s:%u successfully.\n", 
                    BARRIER_SERVER_ADDR, BARRIER_SERVER_PORT);
        
        /* Send request message. */
        request_msg[0] = 0;
        request_msg[1] = 0;
        
        if ((send(client_tcp_fd, request_msg, MSGSIZE*2, 0)) != MSGSIZE*2) {
                perror("send");
                ret = -1;
                goto error;
        }
       
        printf("Request message has been sent out successfully.\n");

        /* Receive base_time and timeout_interval. */
        if (recv(client_tcp_fd, &msg_buffer, MSGSIZE*2, 0) != MSGSIZE*2) {
                perror("recv parameters");
                ret = -1;
                goto error;
        }

        /* Save. */
        base_time = msg_buffer[0];
        timeout_interval = msg_buffer[1];

        printf("Reply message: base_time is %ld and timeout_interval is %ld.\n",
                base_time, timeout_interval);

error:
        /* Close TCP client socket. */
        close(client_tcp_fd);
        
        return ret;
}

static int init_sender_client() {
        int ret = 0;
       
        /* Initialize UDP socket for heartbeat sending.*/
        memset(&hb_server, '0', sizeof(hb_server));
        hb_server.sin_addr.s_addr = inet_addr(HB_SERVER_ADDR);
        hb_server.sin_family = AF_INET;
        hb_server.sin_port = htons(HB_SERVER_PORT);
        
        client_udp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        return ret;
}

int main(int argc, char *argv[]) {
        if (argc != 1) {
	        printf("Usage: ./hb_sender\n");
	        exit(-1);
        }
        
        signal(SIGINT, sig_handler);

        /* Init */
        if (init_sender() < 0) {
               fprintf(stderr, "Error in initialize heartbeat sender.\n");
               exit(-1);
        }

        printf("Heartbeat sender has been initialized successfully.\n");

        /* Start to loop of heartbeat sending. */
        start_send_heartbeat();

        /* Destroy */
        destroy_sender();

        return 0;
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

                int ret = sendto(sockfd, &hb_send_time, MSGSIZE, 0, (struct sockaddr *) &server, sizeof(server));
                printf("upd sent with hb_send_time %ld\n", hb_send_time);

                if(ret <= 0) {
                        fprintf(stderr, "Error: sendto ret is less than 0.\n");
                        close(sockfd);
                        break;
                }
        
                if(ret != MSGSIZE) 
                        printf("Warning: sendto ret is %d!\n", ret);
                
                packet_sent ++;
                if(packet_sent == 10000)
                        break;
                
                // sleep
                struct timespec sleep_ts = sleep_time();
                nanosleep(&sleep_ts, NULL);
        }
        
        long end_t = now();
        printf("Time used to send 10000 packets is %ld\n", end_t - begin_t);
  */
