#ifndef HB_SENDER_KRETPROBE
#define HB_SENDER_KRETPROBE

extern struct kretprobe my_kretprobe;

int kretprobe_init(void);

void kretprobe_exit(void);

long get_send_block_timeout(void);

long get_send_hb_timeout(void);

long is_send_hb_timeout(void);

int block_send(void);
#endif
