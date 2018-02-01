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

#include "hb_common.h"
#include "helper.h"
#include "sender.h"
#include "request_server.h"

#define SIG_TEST 44 

static long const app_id = 1; // Unique positive value.

static int hb_fd;
static struct sockaddr_in hb_server;

static void clear_debugfs();

static void receive_signal(int n, siginfo_t *info, void *unused) {
        printf("signal handler received value %i\n", info->si_int);
}

int init_sender() {
        int configfd;
        char buf[20];
        struct sigaction sig;

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

        memset(&hb_server, '0', sizeof(hb_server));
        hb_server.sin_addr.s_addr = inet_addr(HB_SERVER_ADDR);
        hb_server.sin_family = AF_INET;
        hb_server.sin_port = htons(HB_SERVER_PORT);
        
        hb_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        /* Start request server thread. */
        init_request_server();
        
        printf("Heartbeat sender has been initialized successfully.\n");

        return 0;
}

/* Destroy. */
void destroy_sender() {
        cancel_request_server();
        join_request_server();
        close(hb_fd);
        clear_debugfs();
        printf("Heartbeat sender has been destroyed.\n");
}

/* heartbeat sending. */
void safetimer_send_heartbeat() {
        long hb_msg;
        int count;
        
        hb_msg= 1;
        count = sendto(hb_fd, &hb_msg, MSGSIZE, 0, \
                (struct sockaddr *) &hb_server, sizeof(hb_server));

        if (count != MSGSIZE) 
                perror("heartbeat sendto");

	printf("Heartbeat message %ld has been sent.\n", hb_msg);

}

static void clear_debugfs() {
        FILE *fp;
                    
        if ((fp = fopen("/sys/kernel/debug/st_debugfs/clear", "r+")) == NULL) {
                perror("fopen clear");
        }                 
        fputs("1", fp);
        fclose(fp);
}
