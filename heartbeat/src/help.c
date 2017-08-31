#include <stdio.h> 
#include <string.h>    
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

/* Link from sender. */
extern long base_time;
extern long timeout_interval;

/* Convert time (ms) to timespec. */
struct timespec time_to_timespec(long time_ms) {
        struct timespec ts;
        ts.tv_sec = time_ms / 1000;
        ts.tv_nsec = (time_ms % 1000) * 1.0e6;
        
        return ts;
}

/* Round up to the epoch id that time belongs to. */
long time_to_epoch(long time) {
        time -= base_time;
        return time < 0 ? -1 : (long)(time / timeout_interval + 1);
}

/* Convert epoch id to time. */
long epoch_to_time(long id) {
        return base_time + (id * timeout_interval);
}

/* Return now time. */
long now_time() {
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        
        return spec.tv_sec * 1000 + spec.tv_nsec/1.0e6;
}
