#ifndef HELPER
#define HELPER

/* Convert time (ms) to timespec. */
struct timespec time_to_timespec(long time_ms);

/* Round up to the epoch id that time belongs to. */
long time_to_epoch(long time);

/* Convert epoch id to time. */
long epoch_to_time(long id);

/* Return now time. */
long now_time(); 

#endif
