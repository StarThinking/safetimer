#ifndef DROP
#define DROP

#define NICDROP 100
#define DEVDROP 10
#define QUEUEDROP 1

extern unsigned long queue_dropped_pkt_current;

int init_drop();

void destroy_drop();

int if_drop_happened();

#endif
