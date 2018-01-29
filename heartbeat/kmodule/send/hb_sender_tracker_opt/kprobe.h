#ifndef HB_SENDER_KPROBE
#define HB_SENDER_KPROBE

int kprobe_init(void);

void kprobe_exit(void);

int block_send(int flag);

extern void reset_sent_epoch(void);

extern void public_set_global_sent_epoch(long epoch);

#endif
