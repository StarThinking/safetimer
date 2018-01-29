#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/ktime.h>
#include <linux/kprobes.h>

#include <linux/skbuff.h>
#include <net/ip.h>

static struct kprobe kp;
DEFINE_PER_CPU(long, local_sent_epoch);
static atomic_long_t global_sent_epoch;

static long timeout_interval = 1000;
static long base_time = 1000000;

static struct kprobe kp = {
    .symbol_name    = "dev_hard_start_xmit",
};

static inline long get_max_transfer_delay(void) {
        return 40;
}

static inline long get_max_clock_deviation(void) {
        //return timeout_interval * 0.1;
        return 10;
}

static inline long get_local_sent_epoch(void) {
        return this_cpu_read(local_sent_epoch);
}

static inline long updateget_local_sent_epoch(void) {
        long latest_epoch = atomic_long_read(&global_sent_epoch);
        this_cpu_write(local_sent_epoch, latest_epoch);
        return latest_epoch;
}

static inline long now(void) {
        return ktime_to_ms(ktime_get_real());
}

static inline long epoch_to_time(long id) {
        return base_time + (id * timeout_interval);
}

static inline long is_send_hb_timeout(void) {
        long send_timeout = 0;
        long now_t = 0;

        //send_timeout = get_send_hb_timeout(flag);
        send_timeout = epoch_to_time(atomic_long_read(&global_sent_epoch) + 1) \
		- get_max_transfer_delay() - get_max_clock_deviation();
        now_t = now();

        return now_t - send_timeout;
}

static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    //printk(KERN_INFO "handler_pre: arg1 = %lu, arg2 = %lu, arg3 = %lu\n",
      //                              regs->di, regs->si, regs->dx);
    	/*struct sk_buff *skb = NULL;
        struct iphdr *iph = NULL;
        struct udphdr *uh = NULL;
        u16 dport;
	long i;
	
        if (regs == NULL)
                return 0;

        if ((skb = (struct sk_buff *) regs->di) == NULL)
                return 0;

        if ((iph = ip_hdr(skb)) == NULL)
                return 0;

        if (iph->protocol != IPPROTO_UDP)
                return 0;
        
        uh = (struct udphdr *) skb_transport_header(skb);
        dport = ntohs(uh->dest);

	if (i++ % 100000 == 0)	{
		unsigned short offset;
                unsigned short iphdr_size;
                unsigned short udphdr_size = 8;
		unsigned char *data;

		data = (unsigned char *) iph;
		iphdr_size = iph->ihl << 2;
		offset = iphdr_size + udphdr_size;
		
		if (is_send_hb_timeout() <= 0) {
			atomic_long_add(1, &global_sent_epoch);
		}
	}*/
    	return 0;
}

static int __init kprobe_args_init(void)
{
    int ret;

    printk(KERN_INFO "kprobe_args_init\n");
    
//    kp.addr = (kprobe_opcode_t *) foobar;
    kp.pre_handler = handler_pre;
    ret = register_kprobe(&kp);

    if (ret < 0) {
        printk(KERN_INFO "register_kprobe failed, returned %d\n", ret);
        return ret;
    }
    
    printk(KERN_INFO "Planted kprobe at %p\n", kp.addr);
    
    return 0;
}
 
static void __exit kprobe_args_exit(void)
{
    unregister_kprobe(&kp);
    printk(KERN_INFO "kprobe_args_exit\n");
}
 
module_init(kprobe_args_init);
module_exit(kprobe_args_exit);
 
MODULE_LICENSE("GPL");
