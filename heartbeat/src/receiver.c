#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include <jni.h>

#include "hb_common.h"

#include "receiver.h"

#include "helper.h"
#include "barrier.h"
#include "hb_server.h"
#include "state_server.h"
#include "drop.h"
#include "queue.h"

long base_time = 0;

long timeout_interval = 0;

struct receiver_stats recv_stats;

sem_t init_done;

static FILE *log_fp;
static long read_timeout_interval();
static void show_receiver_stats();

//JNIEXPORT jint JNICALL Java_org_apache_hadoop_hdfs_server_blockmanagement_ReceiverWrapper_get_1state
//(JNIEnv *env, jclass o) {
//        printf("JNICALL is called\n");
//        return node_state;
//}

//JNIEXPORT jint JNICALL Java_org_apache_hadoop_hdfs_server_blockmanagement_ReceiverWrapper_init_1receiver
//(JNIEnv *env, jclass o) {
int init_receiver() {
        int ret = 0;

         /* Set the values of parameters. */
         base_time = now_time();

         /* Get parameter from ./conf. */
         if ((timeout_interval = read_timeout_interval()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to read timeout_interval from conf file.\n");
                ret = -1;
                goto error;
         }

         recv_stats.timeout_interval = timeout_interval;
         
         printf("Heartbeat receiver configuration parameters: ");
         printf("timeout_interval = %ld, base_time = %ld\n", 
                    timeout_interval, base_time);
    
         sem_init(&init_done, 0, 0);

#ifdef CONFIG_DROP
         init_drop();
#endif

         if ((ret = init_queue()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init queue.\n");
                goto error;
         }

#ifdef CONFIG_BARRIER         
         if ((ret = init_barrier()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init barrier.\n");
                goto error;
         }
#endif
         if ((ret = init_hb()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init heartbeat server.\n");
                goto error;
         }
         
         if ((ret = init_state_server()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init state server.\n");
                goto error;
         }
         
         printf("Heartbeat receiver has been initialized successfully.\n");
        
         /* Begin to check expiration until init is done. */
         sem_post(&init_done);

error:
            
         return ret;
}

//JNIEXPORT void JNICALL Java_org_apache_hadoop_hdfs_server_blockmanagement_ReceiverWrapper_destroy_1receiver
//(JNIEnv *env, jclass o) {        
void destroy_receiver() {
    show_receiver_stats();

#ifdef CONFIG_DROP
        destroy_drop();
#endif

#ifdef CONFIG_BARRIER
        cancel_barrier();
        join_barrier();
#endif
      
        cancel_hb();
        join_hb();
        
        cancel_queue();
        join_queue();        
        
        cancel_state_server();
        join_state_server();

        printf("Heartbeat receiver has been destroyed.\n");
}

static long read_timeout_interval() {
        FILE *fp;
        char *ptr;
        long ret;
        int buffer_size = 20;
        char buffer[buffer_size];

        fp = fopen("/root/hb-latency/heartbeat/build/conf/timeout_interval", "r");
        if (fp == NULL) {
                perror("fopen ./conf/timeout_interval");
                fclose(fp);
                ret = -1;
        } else {
                memset(buffer, '\0', buffer_size);
                fgets(buffer, buffer_size, fp);
                ret = strtol(buffer, &ptr, 10);
        }

        fclose(fp);

        return ret;
}

#define SHOW_STAT(val) \
if(1) { \
    printf("%s = %ld\n", #val, recv_stats.val); \
    fprintf(log_fp, "%s = %ld\n", #val, recv_stats.val); \
} 

static void show_receiver_stats() {
        log_fp = fopen("/root/hb-latency/heartbeat/build/receiver.log", "w");
        if (log_fp == NULL) {
                perror("fopen receiver.log");
                fclose(log_fp);
                return;
        }

        printf("Heartbeat receiver statistics have been saved into log.\n");

        SHOW_STAT(timeout_interval);
        SHOW_STAT(hb_cnt);
        SHOW_STAT(waived_timeout_cnt);
        SHOW_STAT(timeout_cnt);
        SHOW_STAT(hb_drop_cnt);
        SHOW_STAT(nic_drop_cnt);
        SHOW_STAT(dev_drop_cnt);
        SHOW_STAT(dev_drop_pkts);
        SHOW_STAT(queue_drop_cnt);
        SHOW_STAT(queue_drop_pkts);

        fclose(log_fp);
        printf("end of show_receiver_stats\n");

        return;
}
