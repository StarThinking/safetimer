#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>

pthread_t tid;
int running = 1;

void sig_handler(int signo) {
    running = 0;
    printf("switched runnning to 0\n");
}

void *func(void *args) {
    while(running) {
        printf("do something in thread\n");
        sleep(10);
    }
    printf("thread exits\n");
}

int main(void) {
    signal(SIGINT, sig_handler);
    
    pthread_create(&tid, NULL, func, NULL);
    printf("thread created\n");

    while(running) {
        printf("do something in main thread\n");
        sleep(10);
    }

    pthread_join(tid, NULL);
    printf("thread joined\n");

    return 0;
}
