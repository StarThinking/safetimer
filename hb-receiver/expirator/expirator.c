#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <semaphore.h>

static long timeout_intvl_ms = 0;

static pthread_t expirator_tid;

/*void sig_handler(int signo) {
    if (signo == SIGINT) {
        pthread_cancel(expirator_tid);
        exit(0);
    }
}*/

long now() {   
    struct timespec spec;    
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec/1.0e6;
}

struct timespec sleep_time(long sleep_t) {
    struct timespec sleep_ts;
    sleep_ts.tv_sec = sleep_t / 1000;
    sleep_ts.tv_nsec = (sleep_t % 1000) * 1.0e6;
    return sleep_ts;
}

// thread for checking expiration
void *expirator(void *arg) {
    long timeout_t = now() + timeout_intvl_ms;;
    struct timespec sleep_ts;
    
    while(1) {
        while(now() < timeout_t) {
            sleep_ts = sleep_time(timeout_t - now());
            nanosleep(&sleep_ts, NULL);
        }
        timeout_t += timeout_intvl_ms;
    }
}

int main(int argc, char *argv[]) {
    
//    signal(SIGINT, sig_handler);
    
    if(argc != 2) {
        printf("./receiver [timeout ms]\n");
	return 1;
    }

    timeout_intvl_ms = atoi(argv[1]);
    printf("timeout = %ld ms\n", timeout_intvl_ms);

    // create expirator thread
    //pthread_create(&expirator_tid, NULL, expirator, &sock);

    FILE *fp;
    char buff[10];
    fp = fopen("/proc/hello", "r");
    while(1) {
        fscanf(fp, "%s", buff);
        int content = atoi(buff);
        if(content == 1)
            break;
        sleep(1);
    }
    printf("/proc/hello : %s\n", buff );
    fclose(fp);
}
