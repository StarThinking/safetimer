#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "hb_config.h"
#include "barrier.h"
#include "hb_server.h"

void sig_handler(int signo) {
        if (signo == SIGINT) {
                destroy_barrier();
                destroy_hb();
                printf("Heartbeat receiver ternimates in sig_handler.\n");
                exit(0);
        }
}

int main(int argc, char *argv[]) {
         int ret = 0;
         
         signal(SIGINT, sig_handler);

         if ((ret = init_barrier()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init barrier.\n");
                goto error;
         }
        
         if ((ret = init_hb()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init heartbeat server.\n");
                goto error;
         }

         ret = send_barrier_message(0);
         ret = send_barrier_message(1);
             
         sleep(30);

error:
         destroy_hb();
         destroy_barrier();
            
         printf("Heartbeat receiver ternimates.\n");
         
         return ret;
}
