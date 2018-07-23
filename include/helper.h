#ifndef HELPER
#define HELPER

/* Convert time (ms) to timespec. */
struct timespec time_to_timespec(long time_ms);

/* Return now time. */
long now_time(); 

#endif
