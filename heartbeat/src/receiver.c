#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "hb_config.h"
#include "barrier.h"
#include "hb_server.h"

void sig_handler(int signo) {
        if (signo == SIGINT) {
                /* Destroy barrier */
                destroy_barrier();

                printf("Heartbeat receiver ternimated!\n");

                exit(0);
        }
}

int main(int argc, char *argv[]) {
         int ret = 0;
         
         signal(SIGINT, sig_handler);

         if ((ret = init_barrier()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init barrier.\n");
                return ret;
         }
        
         if ((ret = init_hb()) < 0) {
                fprintf(stderr, "Heartbeat receiver failed to init heartbeat server.\n");
                return ret;
         }

         ret = send_barrier_message(0);
         ret = send_barrier_message(1);
             
         sleep(30);

         destroy_hb();
         destroy_barrier();
            
         return ret;
}
