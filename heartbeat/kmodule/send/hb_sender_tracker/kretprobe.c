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
#include "../../../include/hb_common.h"

MODULE_LICENSE("GPL");

long now(void) {
        //return ktime_to_ms(ktime_get());
        return ktime_to_ms(ktime_get_real());
}

/* If sent_epoch = 0, return -1 to indicate not prepared. */
long get_send_timeout(void) {
        long sent_epoch, recv_timeout;
        
        /* get_sent_epoch() involves spinlock contention. */
        if ((sent_epoch = get_sent_epoch()) == 0)
                return -1;
        
        recv_timeout = epoch_to_time(sent_epoch + 1);
        
        return recv_timeout - get_max_transfer_delay() - get_max_clock_deviation();
}

long timeout(void) {
        long send_timeout = 0;
        long now_t = 0;
        
        if (!prepared())
                return 0;

        send_timeout = get_send_timeout();

        /* Not prepared. */
        if(send_timeout < 0)
                return 0;
        
        now_t = now();
        
        return now_t - send_timeout;
}

static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
        struct sk_buff *skb = NULL;
        struct iphdr *iph = NULL;
        struct udphdr *uh = NULL;
        unsigned short offset;
        unsigned short iphdr_size;
        unsigned short udphdr_size = 8;

//        if(ri->ret_addr != (void *) 0xffffffffa004a0b5)
//                return 0;
        if (!prepared())
                return 0;

        if (regs != NULL) {
            skb = (struct sk_buff *) regs->di;
            if (skb != NULL) {
                iph = ip_hdr(skb);
                if (iph != NULL && iph->protocol == IPPROTO_UDP) {
                    // change from udp_hdr() to skb_transport_header()
                    uh = (struct udphdr *) skb_transport_header(skb);
                    if (uh != NULL) {
                        u16 dport = ntohs(uh->dest);
                        if (dport == HB_SERVER_PORT) {
                            unsigned char *data;
                            long flag, epoch;
                            long exceeding_time;
                            
                            data = (unsigned char *) iph;
                            iphdr_size = iph->ihl << 2;
                            offset = iphdr_size + udphdr_size;
                            flag = *((long *) (data + offset));
                            epoch = *((long *) (data + offset + MSGSIZE));

                            if ((exceeding_time = timeout()) <= 0) {
                                set_sent_epoch(epoch);
//                                printk("heartbeat sending completes and set sent_epoch as %ld\n", 
//                                        epoch);
                            } else {
                                printk("The completion of heartbeat sending for epoch %ld exceeds sending_timeouts (%ld) by %ld!\n", 
                                        epoch ,get_send_timeout(), exceeding_time);
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
