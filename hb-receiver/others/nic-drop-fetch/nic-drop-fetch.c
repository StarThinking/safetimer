#include <linux/init.h>
#include <linux/module.h>
#include <linux/debugfs.h> /* this is for DebugFS libraries */
#include <linux/fs.h>   
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/io.h>

MODULE_LICENSE("GPL");

#define tr32(reg)   tp->read32(tp, reg)
#define HOSTCC_FLOW_ATTN    0x00003c48
// copied from the drivers/net/ethernet/broadcom/tg3.h


extern void *my_tp_regs[];
extern int my_tp_regs_count;

int init_module() {
        int i;

        for(i = 0; i < my_tp_regs_count; i++) {
//                tg3_stat64_t drop, discard;

                u32 val = readl(my_tp_regs[i] + HOSTCC_FLOW_ATTN);
                printk("%08x\n", val);
                //val = (val & HOSTCC_FLOW_ATTN_MBUF_LWM) ? 1 : 0;
        } 
 
        return 0;
}
 
void cleanup_module() {
}
