#ifndef HB_COMMON
#define HB_COMMON

//#define CONFIG_DROP
#define CONFIG_BARRIER
#define CONFIG_SENDBLOCK

#define ST_HB_SERVER_ADDR "10.0.0.11"
#define ST_HB_SERVER_PORT 5001

#define BARRIER_SERVER_ADDR "10.0.0.11"
#define BARRIER_CLIENT_ADDR "10.0.0.111"
#define BARRIER_SERVER_PORT 5002 
#define BARRIER_SERVER_IF "eno1"
#define BARRIER_CLIENT_IF "eno2"

#define ST_RECV_SERVER_ADDR "127.0.0.1"
#define ST_RECV_SERVER_PORT 5003
#define ST_SEND_SERVER_ADDR "127.0.0.1"
#define ST_SEND_SERVER_PORT 5004

#define SIG_FROM_KERNELMODULE 44

#define ST_RECEIVER_REQUEST_MSGSIZE sizeof(long)*3
#define ST_RECEIVER_REPLY_MSGSIZE sizeof(long)
#define ST_SENDER_REQUEST_MSGSIZE sizeof(long)*3
#define ST_SENDER_REPLY_MSGSIZE sizeof(long)
#define ST_HB_MSGSIZE sizeof(long)

#define MSGSIZE sizeof(long)

#define IRQ_NUM 4
#define BASE_IRQ 33

#ifndef __KERNEL__

#include <semaphore.h>
typedef void (*cb_t)(void);

struct receiver_stats {
        long timeout_interval;
        long hb_cnt;

        long nic_drop_cnt;
        long dev_drop_cnt;
        long queue_drop_cnt;
        /* Any of the above three drop happens, hb_drop_cnt increases. */
        long hb_drop_cnt;
        
        long dev_drop_pkts;
        long queue_drop_pkts;

        long barrier_cnt;
        long timeout_cnt;
};

extern struct receiver_stats recv_stats;

extern sem_t init_done;

#endif // KERNEL

#endif
