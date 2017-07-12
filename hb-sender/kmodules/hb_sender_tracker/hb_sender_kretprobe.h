#ifndef HB_SENDER_KRETPROBE
#define HB_SENDER_KRETPROBE

#include "hb_sender_debugfs.h"

extern long hb_send_compl_time;
extern long udp_send_time;

int kretprobe_init(void);

void kretprobe_exit(void);

long now(void);

long timeout(void);

#endif
