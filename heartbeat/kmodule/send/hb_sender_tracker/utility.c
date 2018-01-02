#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>

#include <linux/skbuff.h>
#include <net/ip.h>

#include "debugfs.h"
#include "utility.h"
#include "../../../include/hb_common.h"

//MODULE_LICENSE("GPL");

long now(void) {
        //return ktime_to_ms(ktime_get());
        return ktime_to_ms(ktime_get_real());
}

/* If sent_epoch = 0, return -1 to indicate not prepared. */
static long get_send_block_timeout(void) {
        long sent_epoch, recv_timeout;
        
        /* get_sent_epoch() involves spinlock contention. */
        if ((sent_epoch = get_sent_epoch()) == 0)
                return -1;
        
        recv_timeout = epoch_to_time(sent_epoch + 1);
        
        return recv_timeout - get_max_clock_deviation();
}

int block_send(void) {
        long now_t = now();
        long send_block_timeout = get_send_block_timeout();

        if (!prepared())
                return 0;

        if (send_block_timeout < 0)
                return 0;
        
    //    printk("now is %ld, send_block_timeout is %ld\n", now_t, send_block_timeout);
        return now_t < send_block_timeout ? 0 : 1;
}
