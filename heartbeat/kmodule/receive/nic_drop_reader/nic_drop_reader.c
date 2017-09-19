#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/hardirq.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <linux/netdevice.h>

#include "../../../include/hb_common.h"

#define HOSTCC_FLOW_ATTN    0x00003c48
#define HOSTCC_FLOW_ATTN_MBUF_LWM   0x00000040

#define BUFFERSIZE 20

MODULE_LICENSE("GPL");

static char accum_nic_drop_str[BUFFERSIZE];
static unsigned long accum_nic_drop;
static struct dentry *dir_entry;
static struct dentry *dropped_packets_entry;

extern void *my_tp_regs[];
extern int my_tp_regs_count;

typedef struct {
        u32 high, low;
} tg3_stat64_t;

static inline u64 get_stat64(tg3_stat64_t *val) {
        return ((u64)val->high << 32) | ((u64)val->low);
}

static ssize_t dropped_packets_read(struct file *fp, char __user *user_buffer,
                                    size_t count, loff_t *position) { 
        unsigned long dropped_packets = 0;
        tg3_stat64_t rx_discards;
        int i;
 
        rx_discards.high = rx_discards.low = 0;
       
        /* 
         * Iterate for each tp. In fact, there may be nicer method to
         * get the reg of a specific tp by calling driver's function.
         */
        for (i = 0; i < my_tp_regs_count; i++) {
                u32 val = readl(my_tp_regs[i] + HOSTCC_FLOW_ATTN);
//                printk("[nic drop reader] val = %08x %u\n", val, val);
                val = (val & HOSTCC_FLOW_ATTN_MBUF_LWM) ? 1 : 0;
//                printk("val = %u\n", val);
                if (val) {
                        writel(HOSTCC_FLOW_ATTN_MBUF_LWM, my_tp_regs[i] + HOSTCC_FLOW_ATTN);
                        rx_discards.low += val;
                        if (rx_discards.low < val)
                                rx_discards.high += 1;
                }
        }            
        dropped_packets = get_stat64(&rx_discards);

        if (dropped_packets > 0) {
                accum_nic_drop += dropped_packets;
                memset(accum_nic_drop_str, '\0', BUFFERSIZE);
                snprintf(accum_nic_drop_str, 10, "%lu", accum_nic_drop);
                
//                printk("[msx] accum_nic_drop is %lu, this time added %lu pkts\n", 
//                            accum_nic_drop, dropped_packets); 
        }

        return simple_read_from_buffer(user_buffer, count, position, accum_nic_drop_str, BUFFERSIZE);
}

static const struct file_operations dropped_packets_fops = {
        .read = dropped_packets_read
};

static int nic_drop_reader_init(void) {
        printk(KERN_INFO "nic_drop_reader init\n");

        memset(accum_nic_drop_str, '\0', BUFFERSIZE);
        
        dir_entry = debugfs_create_dir("nic_drop_reader", NULL);
        dropped_packets_entry = debugfs_create_file("dropped_packets", 0444, dir_entry, NULL, &dropped_packets_fops);

        return 0;
}

static void nic_drop_reader_exit(void) {
        debugfs_remove_recursive(dir_entry);

        printk(KERN_INFO "dev_drop_reader exit\n");
}

module_init(nic_drop_reader_init)
module_exit(nic_drop_reader_exit)
