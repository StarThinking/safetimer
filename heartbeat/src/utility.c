#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/types.h>
#include <time.h>

#include "utility.h"

int ip_equal(void *a, void *b) {
        return 0 == strcmp((char*)a, (char*)b);
}

void free_val(void *val) {
        free(val);
}

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
    
long random_at_most(long max) {
        unsigned long
        // max <= RAND_MAX < ULONG_MAX, so this is okay.
        num_bins = (unsigned long) max + 1,
        num_rand = (unsigned long) RAND_MAX + 1,
        bin_size = num_rand / num_bins,
        defect   = num_rand % num_bins;

        long x;
        do {
                x = random();
        }   
        // This is carefully written not to overflow
        while (num_rand - defect <= (unsigned long)x);

        // Truncated division is intentional
        return x/bin_size;
}