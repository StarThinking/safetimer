#include <stdio.h>
#include <pthread.h>
#include <signal.h>

int i = 0;
pthread_t tid;

void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("SIGINT handled\n");
        pthread_cancel(tid);
    }
}

void *fun(void *arg) {
    printf("i am a thread\n");
    while(1) {
        sleep(1);
        i++;
    }
}

int main () {
    signal(SIGINT, sig_handler);
    void *status;
    pthread_create(&tid, NULL, fun, NULL);
    pthread_join(tid, NULL);
    printf("after join\n");
    return 0;
}
