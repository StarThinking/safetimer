#ifndef HB_SENDER_KRETPROBE
#define HB_SENDER_KRETPROBE

extern struct kretprobe my_kretprobe;

int kretprobe_init(void);

void kretprobe_exit(void);

long get_send_block_timeout(int flag);

//long get_send_hb_timeout(int flag);

//long is_send_hb_timeout(int flag);

int block_send(int flag);
#endif
