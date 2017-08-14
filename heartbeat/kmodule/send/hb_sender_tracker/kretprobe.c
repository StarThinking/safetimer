#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>

#include <linux/skbuff.h>
#include <net/ip.h>

#include "debugfs.h"
#include "kretprobe.h"

MODULE_LICENSE("GPL");

long now(void) {
        //return ktime_to_ms(ktime_get());
        return ktime_to_ms(ktime_get_real());
}

long get_send_timeout(void) {
        // get_hb_send_compl_time() involves spinlock contention
        long hb_epoch = time_to_epoch(get_hb_send_compl_time());
        long recv_timeout = epoch_to_time(hb_epoch + 1);
        
        return recv_timeout - get_max_transfer_delay() - get_max_clock_deviation();
}

long timeout(void) {
        long send_timeout = 0;
        long now_t = 0;
        
//        if(!prepared())
//                return 0;

        send_timeout = get_send_timeout();
        now_t = now();
        
        if(!prepared())
                return 0;
        
        return now_t - send_timeout;
}

static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
        struct sk_buff *skb = NULL;
        struct iphdr *iph = NULL;
        struct udphdr *uh = NULL;

//        if(ri->ret_addr != (void *) 0xffffffffa004a0b5)
//                return 0;
        if(!prepared())
                return 0;

        if(regs != NULL) {
            skb = (struct sk_buff *) regs->di;
            if(skb != NULL) {
                iph = ip_hdr(skb);
                if(iph != NULL && iph->protocol == IPPROTO_UDP) {
                    uh = udp_hdr(skb);
                    if(uh != NULL) {
                        u16 dport = ntohs(uh->dest);
                        if(dport == 5001) {
                            unsigned char *data = (unsigned char *) iph;
                            long *hb_send_time_p = (long *) (data + 28);
                            long hb_send_time = *hb_send_time_p;

                            long exceeding_time = timeout();
                            if(exceeding_time <= 0) {
                                //printk("hb send completes and update hb_send_compl_time %ld to hb_send_time %ld\n", hb_send_compl_time, hb_send_time);
                                set_hb_send_compl_time(hb_send_time);
                            } else {
                                printk("hb send completion timeouts (%ld) by %ld!\n", get_send_timeout(), exceeding_time);
                            } 
                        }
                    }
                }
            }
        }
        
        return 0;
}

static struct kretprobe my_kretprobe = {
	.entry_handler		= entry_handler,
	// Probe up to 20 instances concurrently. 
	.maxactive		= 20,
};

int kretprobe_init(void) {
	int ret;

	my_kretprobe.kp.symbol_name = "consume_skb";
	ret = register_kretprobe(&my_kretprobe);
	if (ret < 0) {
		printk(KERN_INFO "register_kretprobe failed, returned %d\n", ret);
		return -1;
	}
	printk(KERN_INFO "Planted return probe at %s: %p\n",
			my_kretprobe.kp.symbol_name, my_kretprobe.kp.addr);
      
        return 0;
}

void kretprobe_exit(void) {
        unregister_kretprobe(&my_kretprobe);
	printk(KERN_INFO "kretprobe at %p unregistered\n",
			my_kretprobe.kp.addr);

	// nmissed > 0 suggests that maxactive was set too low. 
	printk(KERN_INFO "Missed probing %d instances of %s\n",
		my_kretprobe.nmissed, my_kretprobe.kp.symbol_name);
}
