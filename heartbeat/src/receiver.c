#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "receiver.h"

#include "hb_config.h"
#include "helper.h"
#include "barrier.h"
#include "hb_server.h"
#include "queue.h"

extern long base_time;
long base_time = 0;

extern long timeout_interval;
long timeout_interval = 0;

extern sem_t init_done;
sem_t init_done;

int init_receiver(cb_t callback) {
         int ret = 0;

         /* Set the valus of parameters. */
         base_time = now_time();
         timeout_interval = 1000;
    
         sem_init(&init_done, 0, 0);

         if ((ret = init_queue(callback)) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init queue.\n");
                goto error;
         }
         
         if ((ret = init_barrier()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init barrier.\n");
                goto error;
         }
        
         if ((ret = init_hb()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init heartbeat server.\n");
                goto error;
         }
         
         printf("Heartbeat receiver has been initialized successfully.\n");
        
         /* Begin to check expiration until init is done. */
         sem_post(&init_done);

error:
            
         return ret;
}

void destroy_receiver() {
        cancel_barrier();
        join_barrier();
        
        cancel_hb();
        join_hb();
        
        cancel_queue();
        join_queue();

        printf("Heartbeat receiver has been destroyed.\n");
}

void sig_handler(int signo) {
        if (signo == SIGINT) {
                destroy_receiver();
                printf("Heartbeat receiver ternimates in sig_handler.\n");
                exit(0);
        }
}

void callback() {
        printf("This is a callback function.\n");
        return;
}

int main(int argc, char *argv[]) {
        signal(SIGINT, sig_handler);

        init_receiver(callback);

        while(1)
                sleep(1);

        destroy_receiver();

        return 0;
}
