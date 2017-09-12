#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "sender.h"

void sig_handler(int signo) {
        destroy_sender();
        printf("Heartbeat sender ternimates in sig_handler.\n");
        exit(0);
}

int main(int argc, char *argv[]) {
        signal(SIGINT, sig_handler);
        signal(SIGTERM, sig_handler);
        signal(SIGKILL, sig_handler);

        init_sender();

        while (1) {
                printf("main thread sleeps.\n");
                sleep(1);
        };

        destroy_sender();

        return 0;
}
