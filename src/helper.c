#include <stdio.h> 
#include <string.h>    
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "hb_common.h"

/* Convert time (ms) to timespec. */
struct timespec time_to_timespec(long time_ms) {
        struct timespec ts;
        ts.tv_sec = time_ms / 1000;
        ts.tv_nsec = (time_ms % 1000) * 1.0e6;
        
        return ts;
}

/* Return now time. */
long now_time() {
        struct timespec spec;
        clock_gettime(CLOCK_MONOTONIC, &spec);
        
        return spec.tv_sec * 1000 + spec.tv_nsec/1.0e6;
}
