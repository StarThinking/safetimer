#include <linux/init.h>
#include <linux/module.h>
#include <linux/debugfs.h> /* this is for DebugFS libraries */
#include <linux/fs.h>   
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/io.h>

MODULE_LICENSE("GPL");

// copied from the drivers/net/ethernet/broadcom/tg3.h
#define RCVLPC_NO_RCV_BD_CNT 0x0000224c

#define BUF_LEN 200

static struct dentry *dir, *file;

typedef struct {
        u32 high, low;
} tg3_stat64_t;

static inline u64 get_stat64(tg3_stat64_t *val) {
        return ((u64)val->high << 32) | ((u64)val->low);
}

#define TG3_STAT_ADD32(PSTAT, REGS, OFF) \
do {    u32 __val = ioread32(REGS + OFF); \
        (PSTAT)->low += __val; \
        if ((PSTAT)->low < __val) \
        (PSTAT)->high += 1; \
} while (0)

extern void *my_tp_regs[];
extern int my_tp_regs_count;

/* read file operation */
static ssize_t my_reader(struct file *fp, char __user *user_buffer,
                                size_t count, loff_t *position) {
        u64 pkt_dropped = 0;
        int i;
        char kern_buf[BUF_LEN] = "";

        // Read the value of RCVLPC_NO_RCV_BD_CNT from the I/O memory.
        // It is needed to "disable" the reading of this value by NIC driver.
        // Currently, we acculumate the sum of dropped packets by all nic devices,
        // as it is hard to identify by filename. It is not necessary but safe. 
        for(i = 0; i < my_tp_regs_count; i++) {
                tg3_stat64_t rxbds_empty;

                rxbds_empty.high = rxbds_empty.low = 0;
                TG3_STAT_ADD32(&rxbds_empty, my_tp_regs[i], RCVLPC_NO_RCV_BD_CNT);
                //printk("rxbds_empty = %llu\n", get_stat64(&rxbds_empty));
                pkt_dropped += get_stat64(&rxbds_empty);
        }        
        
        sprintf(kern_buf, "%llu\n", pkt_dropped);

        return simple_read_from_buffer(user_buffer, count, position, kern_buf, BUF_LEN);
}
 
static const struct file_operations fops = {
        .read = my_reader
};

int init_module() {
    dir = debugfs_create_dir("nic-drop", NULL);
    file = debugfs_create_file("pkt_dropped", 0644, dir, NULL, &fops);
 
    return 0;
}
 
void cleanup_module() {
    debugfs_remove_recursive(dir);
}
