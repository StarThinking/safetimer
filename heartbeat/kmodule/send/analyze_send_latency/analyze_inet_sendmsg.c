#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

#define inet_sendmsg_tlb_max 100
extern u64 inet_sendmsg_tlb[];
extern int inet_sendmsg_tlb_index;
extern spinlock_t inet_sendmsg_tlb_lock;
static u64 copied_inet_sendmsg_tlb[inet_sendmsg_tlb_max];

#define send_check_tlb_max 100
extern u64 send_check_tlb[];
extern int send_check_tlb_index;
extern spinlock_t send_check_tlb_lock;
static u64 copied_send_check_tlb[send_check_tlb_max];

static int __init analyze_inet_sendmsg_init(void) {
        int i;

        printk("analyze_inet_sendmsg_init\n");
        printk("inet_sendmsg_tlb_index = %d, send_check_tlb_index = %d\n",
                inet_sendmsg_tlb_index, send_check_tlb_index);

        spin_lock(&inet_sendmsg_tlb_lock);
        memcpy(copied_inet_sendmsg_tlb, inet_sendmsg_tlb, inet_sendmsg_tlb_max * sizeof(u64));
        spin_unlock(&inet_sendmsg_tlb_lock);

        spin_lock(&send_check_tlb_lock);
        memcpy(copied_send_check_tlb, send_check_tlb, send_check_tlb_max * sizeof(u64));
        spin_unlock(&send_check_tlb_lock);
        
        for(i=0; i<inet_sendmsg_tlb_max; i++) {
                printk("index %d inet_sendmsg latency is %llu\n", i, copied_inet_sendmsg_tlb[i]);
        }
        
        for(i=0; i<send_check_tlb_max; i++) {
                printk("index %d send_check latency is %llu\n", i, copied_send_check_tlb[i]);
        }
        
        return 0;
}

static void __exit analyze_inet_sendmsg_exit(void) {
        printk("analyze_inet_sendmsg_exit\n");
}

module_init(analyze_inet_sendmsg_init)
module_exit(analyze_inet_sendmsg_exit)
