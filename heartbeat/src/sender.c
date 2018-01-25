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

//#include <jni.h>

#include "hb_common.h"
#include "helper.h"

#include "sender.h"

long base_time = 0;
long timeout_interval = 0;

static long const app_id = 1; // Unique positive value.

static int hb_fd;
static struct sockaddr_in hb_server;

static pthread_t tid;
static void *run_hb_loop(void *arg);
static void debugfs_save(long base_time, long timeout_interval);    
static void clear_debugfs();
static void enable_intercept();

int init_sender() {

//JNIEXPORT jint JNICALL Java_org_apache_hadoop_hdfs_server_datanode_SenderWrapper_init_1sender
//(JNIEnv *env, jclass jobj) {
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
       
        printf("Heartbeat sender: request message [0, 0] has been sent.\n");
        
        /* Receive base_time and timeout_interval. */
        memset(&remote, '0', sizeof(remote));
        len = sizeof(remote);
        
        if (recvfrom(hb_fd, msg_buffer, MSGSIZE*2, 0, (struct sockaddr *) &remote, &len) != MSGSIZE*2) {
                perror("recvfrom");
                ret = -1;
                goto error;
        }
        
        /* Get parameters. */
        base_time = msg_buffer[0];
        timeout_interval = msg_buffer[1];
        
        if (base_time <= 0 || timeout_interval <=0) {
                fprintf(stderr, "Error values of base_time or timeout_interval.\n");
                ret = -1;
                goto error;
        }
        
        printf("Reply message [base_time=%ld, timeout_interval=%ld] is received.\n",
                base_time, timeout_interval);

#ifdef CONFIG_SENDBLOCK
        /* Save parameters into debugfs so that hb_sender_tracker can read them. */
        debugfs_save(base_time, timeout_interval);
#endif

        /* Start sending thread. */
        pthread_create(&tid, NULL, run_hb_loop, NULL);
        
        printf("Heartbeat sender has been initialized successfully.\n");

error:
        return ret;
}

/* Destroy. */
void destroy_sender() {
//JNIEXPORT void JNICALL Java_org_apache_hadoop_hdfs_server_datanode_SenderWrapper_destroy_1sender
//(JNIEnv *env, jclass jobj) {
        pthread_cancel(tid);
        pthread_join(tid, NULL);
        
#ifdef CONFIG_SENDBLOCK
        clear_debugfs();
        printf("Debugfs cleared.\n");
#endif

        printf("Heartbeat sender has been destroyed.\n");
}

static void cleanup(void *arg) {
        printf("Clean up.\n");
        close(hb_fd);
}

/* Loop of heartbeat sending. */
static void *run_hb_loop(void *arg) {
        int count;
        long hb_msg[2]; /* [app_id, epoch_id] */
        long current_epoch, sending_epoch;
        long diff_time;
        long timeout;
#ifdef CONFIG_SENDBLOCK
        int first_hb = 1;
#endif
        //int count_tmp = 0;
        
        pthread_cleanup_push(cleanup, NULL);

        /* Set the first long to indicate app_id, which should be positive. */
        hb_msg[0] = app_id; 
        
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

                /* 
                 * Before trying to send the FIRST heartbeat, set the value of "sent_epoch" as sending_epoch - 1.
                 * So that this heartbeat sending should be completed before sending timeout, 
                 * which is time of epoch - transmision time - clock diff.
                 */
#ifdef CONFIG_SENDBLOCK
                if (first_hb) {
                        FILE *fp;
                        char *buf;
                        long sent_epoch = sending_epoch -1;

                        if ((fp = fopen("/sys/kernel/debug/hb_sender_tracker/sent_epoch", "r+")) == NULL) {
                                perror("fopen sent_epoch");
                                fclose(fp);
                                break;
                        }

                        buf = (char*) calloc(20, sizeof(char));
                        sprintf(buf, "%ld", sent_epoch);
                        fputs(buf, fp);
                        free(buf);
                        fclose(fp);
                        first_hb = 0;
                        
                        printf("Update sent_epoch as %ld before sending the first heartbeat.\n", sent_epoch);
                }
#endif
		enable_intercept();
                count = sendto(hb_fd, &hb_msg, MSGSIZE*2, 0, (struct sockaddr *) &hb_server, 
                        sizeof(hb_server));

                if (count != MSGSIZE*2) {
                        perror("heartbeat sendto");
                        break;
                }
                
                printf("Heartbeat message [app_id=%ld, epoch=%ld] has been sent.\n", 
                            hb_msg[0], hb_msg[1]);

                /* Adding delay to test kernel module. */
                /*count_tmp ++;
                if (count_tmp == 60) {
                        struct timespec ts = time_to_timespec(timeout_interval);
                        nanosleep(&ts, NULL);
                }*/
        }

        pthread_cleanup_pop(1);
        
        return NULL;
}

static void debugfs_save(long base_time, long timeout_interval) {
        FILE *fp1, *fp2;
        char *buf;

        if ((fp1 = fopen("/sys/kernel/debug/hb_sender_tracker/base_time", "r+")) == NULL) {
                perror("fopen base_time");
                fclose(fp1);
                goto error;
        }

        buf = (char*) calloc(20, sizeof(char));
        sprintf(buf, "%ld", base_time);
        fputs(buf, fp1);
        free(buf);
        fclose(fp1);
        
        if ((fp2 = fopen("/sys/kernel/debug/hb_sender_tracker/timeout_interval", "r+")) == NULL) {
                perror("fopen timeout_interval");
                fclose(fp2);
                goto error;
        }

        buf = (char*) calloc(20, sizeof(char));
        sprintf(buf, "%ld", timeout_interval);
        fputs(buf, fp2);
        free(buf);
        fclose(fp2);

error:
        return;
}

static void clear_debugfs() {
        FILE *fp;
        
        if ((fp = fopen("/sys/kernel/debug/hb_sender_tracker/clear", "r+")) == NULL) {
                perror("fopen clear");
        }
        
        fputs("1", fp);
        fclose(fp);
}

static void enable_intercept() {
        FILE *fp;
        char *buf;
	char *ptr;
        long enable = 1;
        long enable_read;
        struct timespec ts = time_to_timespec(50); // 50ms
        
        if ((fp = fopen("/sys/kernel/debug/hb_sender_tracker/enable", "r+")) == NULL) {
                perror("fopen clear");
		printf("enable intercept failed!\n");
		return;
        }
        buf = (char*) calloc(20, sizeof(char));
        sprintf(buf, "%ld", enable);
        fputs(buf, fp);

	while (1) {
		memset(buf, '\0', 20);
		fgets(buf, 20, fp);
		enable_read = (long) strtol(buf, &ptr, 10);
		if (enable_read == 1) {
			printf("intercept has been enabled succesfully\n");
			break;
		} 
                nanosleep(&ts, NULL);		
	}
	free(buf);
        fclose(fp);
}
