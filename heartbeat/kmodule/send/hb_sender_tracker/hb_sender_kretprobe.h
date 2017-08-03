#ifndef HB_SENDER_KRETPROBE
#define HB_SENDER_KRETPROBE

int kretprobe_init(void);

void kretprobe_exit(void);

long get_send_timeout(void);

long timeout(void);

#endif
