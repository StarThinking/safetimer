#ifndef HB_COMMON
#define HB_COMMON

//#define CONFIG_DROP
#define CONFIG_BARRIER
#define CONFIG_SENDBLOCK

#define HB_SERVER_ADDR "10.0.0.11"
#define HB_SERVER_PORT 5001
#define STATE_SERVER_ADDR HB_SERVER_ADDR
#define STATE_SERVER_PORT 5003
#define BARRIER_SERVER_ADDR HB_SERVER_ADDR
#define BARRIER_SERVER_PORT 5002 
#define BARRIER_SERVER_IF "eno1"

#define BARRIER_CLIENT_ADDR "10.0.0.111"
#define BARRIER_CLIENT_IF "eno2"

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
