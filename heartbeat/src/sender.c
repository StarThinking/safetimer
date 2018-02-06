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
#include <fcntl.h>
#include <semaphore.h>

#include "hb_common.h"
#include "helper.h"
#include "sender.h"
#include "request_server.h"

#define SIG_TEST 44 
static sem_t sent_wait;

static long const app_id = 1; // Unique positive value.

static int peer_fd0, peer_fd1;
static struct sockaddr_in hb_peer0, hb_peer1;

static void clear_debugfs();

static void receive_signal(int n, siginfo_t *info, void *unused) {
//        printf("signal handler received value %i\n", info->si_int);
        sem_post(&sent_wait);
}

int init_sender() {
        int configfd;
        char buf[20];
        struct sigaction sig;

        sem_init(&sent_wait, 0, 0);
        sig.sa_sigaction = receive_signal;
        sig.sa_flags = SA_SIGINFO;
        sigaction(SIG_TEST, &sig, NULL);
        configfd = open("/sys/kernel/debug/st_debugfs/st_pid", O_WRONLY);
        if(configfd < 0) {
                perror("open");
        }
        sprintf(buf, "%i", getpid());
        if (write(configfd, buf, strlen(buf) + 1) < 0) {
                perror("fwrite");
        }

        memset(&hb_peer0, '0', sizeof(hb_peer0));
        hb_peer0.sin_addr.s_addr = inet_addr("10.10.1.2");
        hb_peer0.sin_family = AF_INET;
        hb_peer0.sin_port = htons(HB_SERVER_PORT);
        peer_fd0 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        
        memset(&hb_peer1, '0', sizeof(hb_peer1));
        hb_peer1.sin_addr.s_addr = inet_addr("10.10.1.3");
        hb_peer1.sin_family = AF_INET;
        hb_peer1.sin_port = htons(HB_SERVER_PORT);
        peer_fd1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        /* Start request server thread. */
        init_request_server();
        
        printf("Heartbeat sender has been initialized successfully.\n");

        return 0;
}

/* Destroy. */
void destroy_sender() {
        cancel_request_server();
        join_request_server();
        close(peer_fd0);
        close(peer_fd1);
        clear_debugfs();
        printf("Heartbeat sender has been destroyed.\n");
}

/* heartbeat sending. */
int safetimer_send_heartbeat(long timeout_time, int node) {
        long hb_msg;
        int count;
        struct timespec wait_timeout;
        int s;
        
        wait_timeout = time_to_timespec(timeout_time);
        hb_msg= 1;
        if (node == 0)
            count = sendto(peer_fd0, &hb_msg, MSGSIZE, 0, \
                    (struct sockaddr *) &hb_peer0, sizeof(hb_peer0));
        
        if (node == 1)
            count = sendto(peer_fd1, &hb_msg, MSGSIZE, 0, \
                    (struct sockaddr *) &hb_peer1, sizeof(hb_peer1));

        if (count != MSGSIZE) 
                perror("heartbeat sendto");

	printf("Heartbeat message %ld has been sent to node %d and now wait for completion signal.\n", 
                hb_msg, node);
    
        // wait for signal
        while ((s = sem_timedwait(&sent_wait, &wait_timeout)) == -1 && errno == EINTR) 
                continue; // Restart if interrupted by handler 

        if (s == -1) {
                if (errno == ETIMEDOUT)
                        printf("sem_timedwait() timed out\n");
                else
                        perror("sem_timedwait");
                return -1;
        } else {
                //printf("sem_timedwait() succeeded\n");
                printf("signal received.");
        }

        return 0;
}

void update_st_valid_time(long t) {
        FILE *fp;
        char buffer[20];
        
        if ((fp = fopen("/sys/kernel/debug/st_debugfs/st_valid_time", "r+")) == NULL) {
                perror("fopen st_valid_time");
        }
        memset(buffer, '\0', 20);
        sprintf(buffer, "%ld", t);
        fputs(buffer, fp);
        fclose(fp);
        printf("st_valid_time has been updated to %ld\n", t);
}

static void clear_debugfs() {
        FILE *fp;
                    
        if ((fp = fopen("/sys/kernel/debug/st_debugfs/clear", "r+")) == NULL) {
                perror("fopen clear");
        }                 
        fputs("1", fp);
        fclose(fp);
}
