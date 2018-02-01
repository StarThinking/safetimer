#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "receiver.h"

void sig_handler(int signo) {
        destroy_receiver();
        printf("Heartbeat receiver ternimates in sig_handler.\n");
        exit(0);
}

int main(int argc, char *argv[]) {
        signal(SIGINT, sig_handler);
        signal(SIGTERM, sig_handler);
        signal(SIGKILL, sig_handler);

        if (init_receiver() == 0) {
                while(1) {
                        sleep(1);
                }
        }

        destroy_receiver();

        return 0;
}
