#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>
#include <linux/skbuff.h>
#include <net/ip.h>

//static char func_name[NAME_MAX] = "__dev_kfree_skb_any";
static char func_name[NAME_MAX] = "napi_consume_skb";
module_param_string(func, func_name, NAME_MAX, S_IRUGO);
MODULE_PARM_DESC(func, "Function to kretprobe; this module will report the"
                    " function's execution time");

/* Here we use the entry_hanlder to timestamp function entry */
static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct sk_buff *skb = NULL;
    struct iphdr *iph = NULL;	
    unsigned int cpu = smp_processor_id();
    printk("entry_handler, cpu id = %u\n", cpu);
	
    if (regs != NULL) {
        skb = (struct sk_buff *) regs->di;
        if (skb != NULL) {
            unsigned int saddr, daddr;
            iph = ip_hdr(skb);
            saddr = (unsigned int) iph->saddr;
            daddr = (unsigned int) iph->daddr;
            printk("iph->protocol = %u, saddr %pI4 --> daddr %pI4\n", iph->protocol, &saddr, &daddr);       
        }
    }   
    return 0;
}

/*
 *  * Return-probe handler: Log the return value and duration. Duration may turn
 *   * out to be zero consistently, depending upon the granularity of time
 *    * accounting on the platform.
 *     */

static struct kretprobe my_kretprobe = {
    .entry_handler      = entry_handler,
    /* Probe up to 20 instances concurrently. */
    .maxactive      = 20,
};

static int __init kretprobe_init(void)
{
    int ret;

    my_kretprobe.kp.symbol_name = func_name;
    ret = register_kretprobe(&my_kretprobe);
    if (ret < 0) {
        printk(KERN_INFO "register_kretprobe failed, returned %d\n", ret);
        return -1;
    }
    printk(KERN_INFO "Planted return probe at %s: %p\n",
        my_kretprobe.kp.symbol_name, my_kretprobe.kp.addr);
    return 0;
}

static void __exit kretprobe_exit(void)
{
    unregister_kretprobe(&my_kretprobe);
    printk(KERN_INFO "kretprobe at %p unregistered\n", my_kretprobe.kp.addr);

    /* nmissed > 0 suggests that maxactive was set too low. */
    printk(KERN_INFO "Missed probing %d instances of %s\n",
    my_kretprobe.nmissed, my_kretprobe.kp.symbol_name);
}

module_init(kretprobe_init)
module_exit(kretprobe_exit)
MODULE_LICENSE("GPL");
