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
         int ret;
         
         signal(SIGINT, sig_handler);

         ret = init_barrier();
         if (ret < 0)
                return -1;
    
         ret = send_barrier_message(0);
         //sleep(1); 
            
         ret = send_barrier_message(1);
         sleep(1);
             
         sleep(30);
         destroy_barrier();
            
         return 0;
}
