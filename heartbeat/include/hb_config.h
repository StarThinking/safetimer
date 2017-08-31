#ifndef HB_CONFIG
#define HB_CONFIG

#define HB_SERVER_ADDR "10.0.0.11"
#define HB_SERVER_PORT 5001
#define BARRIER_SERVER_ADDR HB_SERVER_ADDR
#define BARRIER_SERVER_PORT 5002 
#define BARRIER_SERVER_IF "em1"

#define BARRIER_CLIENT_ADDR "10.0.0.111"
#define BARRIER_CLIENT_IF "em2"

#define MSGSIZE sizeof(long)

#define IRQ_NUM 4
#define BASE_IRQ 49

#endif
