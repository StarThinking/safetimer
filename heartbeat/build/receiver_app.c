#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "../include/receiver.h"

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
