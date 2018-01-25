#include <linux/spinlock.h>

#ifndef HB_SENDER_DEBUGFS
#define HB_SENDER_DEBUGFS

#define BUFFERSIZE 20

extern long base_time;
extern long timeout_interval; // unit of ms
extern long enable;

static inline long get_max_transfer_delay(void) { 
        return 40;
}

static inline long get_max_clock_deviation(void) {
        //return timeout_interval * 0.1;
	return 10;
}

static inline long time_to_epoch(long time) {
        if(timeout_interval == 0)
                return -1;
        
        time -= base_time;
        return time < 0 ? -1 : (long)(time / timeout_interval + 1);
}
           
static inline long epoch_to_time(long id) {
        return base_time + (id * timeout_interval);
}

static inline int prepared(void) {
        return (base_time > 0 && timeout_interval > 0) ? 1 : 0;
}

void disable_intercept(void);

long get_sent_epoch(void);

void inc_sent_epoch(long epoch);

void debugfs_init(void);

void debugfs_exit(void);

#endif
