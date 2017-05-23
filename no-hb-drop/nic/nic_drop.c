#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h> 
#include <linux/moduleparam.h>

#include <linux/io.h>

MODULE_LICENSE("GPL");

#define RCVLPC_NO_RCV_BD_CNT    0x0000224c

static void *tpregs = (void*) 0xffffc90009e00000;

typedef struct {   
        u32 high, low;
} tg3_stat64_t;

static inline u64 get_stat64(tg3_stat64_t *val) 
{               
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

int init_module()
{
        int index = 0;
        
        for(; index < my_tp_regs_count; index++) {
                tg3_stat64_t rxbds_empty;
                
                rxbds_empty.high = rxbds_empty.low = 0;
                
                printk("nic_drop loaded, the address is %p\n", my_tp_regs[index]);
                
                TG3_STAT_ADD32(&rxbds_empty, my_tp_regs[index], RCVLPC_NO_RCV_BD_CNT);
                printk("rxbds_empty = %llu\n", get_stat64(&rxbds_empty));
        }
        return 0; 
}

void cleanup_module() 
{
        printk("nic_drop unloaded\n");
}
