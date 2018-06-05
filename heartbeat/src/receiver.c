#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>


#include "hb_common.h"

#include "receiver.h"
#include "helper.h"
#include "hb_server.h"
#include "state_server.h"
#include "queue.h"

#include "barrier.h"

int init_receiver() {
         int ret = 0;

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
        
error:
         return ret;
}

void destroy_receiver() {
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
