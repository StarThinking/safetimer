#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>

#include <linux/skbuff.h>
#include <net/ip.h>

#include "hb_sender_kretprobe.h"
#include "hb_sender_debugfs.h"

MODULE_LICENSE("GPL");

long hb_send_compl_time = 0;
long udp_send_time = 0;

static long now(void) {
        return ktime_to_ms(ktime_get());
}

long timeout(void) {
        long hb_send_epoch = time_to_epoch(hb_send_compl_time);
        long timeout_recv = epoch_to_time(hb_send_epoch + 1);
        long timeout_send = timeout_recv - get_max_transfer_delay();

        return now() > timeout_send ? 1 : 0;
}

static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
        struct sk_buff *skb = NULL;
        struct iphdr *iph = NULL;
        struct udphdr *uh = NULL;

//        if(ri->ret_addr != (void *) 0xffffffffa004a0b5)
//                return 0;
       // if(!prepared())
       //         return 0;

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
                            long *udp_send_time_p = (long *) (data + 28);
                            udp_send_time = *udp_send_time_p;

                            //if(timeout()) {
                            //  printk("heartbeat sending completion is timeouted!\n");
                            //} else {
                                hb_send_compl_time = now();
                            //}
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
