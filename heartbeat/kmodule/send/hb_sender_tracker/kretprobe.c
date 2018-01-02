#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>

#include <linux/skbuff.h>
#include <net/ip.h>

#include "debugfs.h"
#include "kretprobe.h"
#include "../../../include/hb_common.h"

//MODULE_LICENSE("GPL");

long now(void) {
        //return ktime_to_ms(ktime_get());
        return ktime_to_ms(ktime_get_real());
}

/* If sent_epoch = 0, return -1 to indicate not prepared. */
long get_send_hb_timeout(void) {
        long sent_epoch, recv_timeout;
        
        /* get_sent_epoch() involves spinlock contention. */
        if ((sent_epoch = get_sent_epoch()) == 0)
                return -1;
        
        recv_timeout = epoch_to_time(sent_epoch + 1);
        
        return recv_timeout - get_max_transfer_delay() - get_max_clock_deviation();
}

long get_send_block_timeout(void) {
        return get_send_hb_timeout() + get_max_transfer_delay();
}

int block_send(void) {
        long now_t = now();
        long send_block_timeout = get_send_block_timeout();

        if (!prepared())
                return 0;

        if (send_block_timeout < 0)
                return 0;
        
    //    printk("now is %ld, send_block_timeout is %ld\n", now_t, send_block_timeout);
        return now_t < send_block_timeout ? 0 : 1;
}

long is_send_hb_timeout(void) {
        long send_timeout = 0;
        long now_t = 0;
        
        if (!prepared())
                return 0;

        send_timeout = get_send_hb_timeout();

        /* Not prepared. */
        if (send_timeout < 0)
                return 0;
        
        now_t = now();
        
        return now_t - send_timeout;
}

extern long prev_sent_epoch;
long prev_sent_epoch;

static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
        struct sk_buff *skb = NULL;
        struct iphdr *iph = NULL;
        struct udphdr *uh = NULL;
        unsigned short offset;
        unsigned short iphdr_size;
        unsigned short udphdr_size = 8;

//        if(ri->ret_addr != (void *) 0xffffffffa004a0b5)
//                return 0;
	printk("here1\n");
        if (!prepared())
                return 0;
	printk("here2\n");

        if (regs != NULL) {
            skb = (struct sk_buff *) regs->di;
            if (skb != NULL) {
                iph = ip_hdr(skb);
                if (iph != NULL && iph->protocol == IPPROTO_UDP) {
                    // change from udp_hdr() to skb_transport_header()
                    uh = (struct udphdr *) skb_transport_header(skb);
                    if (uh != NULL) {
                        u16 dport = ntohs(uh->dest);
			printk("dport = %u\n", dport);
                        if (dport == HB_SERVER_PORT) {
			    printk("@!@!@!@\n");
                            unsigned char *data;
                            long flag, epoch;
                            long exceeding_time;
                            long epoch_inc;
                            
                            data = (unsigned char *) iph;
                            iphdr_size = iph->ihl << 2;
                            offset = iphdr_size + udphdr_size;
                            flag = *((long *) (data + offset));
                            epoch = *((long *) (data + offset + MSGSIZE));

                            if ((exceeding_time = is_send_hb_timeout()) <= 0) {
                                epoch_inc = epoch - prev_sent_epoch;
                                inc_sent_epoch(epoch_inc);
				printk("inc_sent_epoch\n");
//                                printk("heartbeat sending completes and set sent_epoch as %ld, int %ld, prev %ld\n", 
 //                                       epoch, epoch_inc, prev_sent_epoch);
                            } else {
                                printk("The completion of heartbeat sending for epoch %ld exceeds sending_timeouts (%ld) by %ld!\n", 
                                        epoch ,get_send_hb_timeout(), exceeding_time);
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
