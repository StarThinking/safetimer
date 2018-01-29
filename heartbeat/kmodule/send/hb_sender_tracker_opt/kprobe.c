#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>

#include <linux/skbuff.h>
#include <net/ip.h>

#include "debugfs.h"
#include "kprobe.h"
#include "../../../include/hb_common.h"

//MODULE_LICENSE("GPL");

DEFINE_PER_CPU(long, local_sent_epoch);
static atomic_long_t global_sent_epoch;

static inline long updateget_local_sent_epoch(void);
static inline long now(void);
static inline long is_send_hb_timeout(void);

extern long base_time;
extern long timeout_interval;

static struct kprobe kp = {
    .symbol_name    = "napi_consume_skb", //__dev_kfree_skb_irq
};

void inline public_set_global_sent_epoch(long epoch) {
	atomic_long_set(&global_sent_epoch, epoch);
}

void inline reset_sent_epoch(void) {
	this_cpu_write(local_sent_epoch, 0);
        atomic_long_set(&global_sent_epoch, 0);
}

inline int block_send(int flag) {
        long now_t = now();
	long sent_epoch;
	long send_block_timeout;

	if (base_time <= 0 || timeout_interval <= 0)
		return 0;
	
	if (likely(flag == 0)) 
		sent_epoch = this_cpu_read(local_sent_epoch);
	else if (unlikely(flag == 1)) 
		sent_epoch = updateget_local_sent_epoch();
	
	send_block_timeout = epoch_to_time(sent_epoch + 1) - get_max_clock_deviation();
	
        return now_t < send_block_timeout ? 0 : 1;
}

static inline long updateget_local_sent_epoch(void) {
	long latest_epoch = atomic_long_read(&global_sent_epoch);
	this_cpu_write(local_sent_epoch, latest_epoch);
	return latest_epoch;
}

static inline long now(void) {
        return ktime_to_ms(ktime_get_real());
}

static inline long is_send_hb_timeout(void) {
        long send_timeout = 0;
        long now_t = 0;
       	   
	send_timeout = epoch_to_time(atomic_long_read(&global_sent_epoch) + 1) \
                - get_max_transfer_delay() - get_max_clock_deviation();
        now_t = now(); 
        
	return now_t - send_timeout;
}

struct dev_kfree_skb_cb {
	enum skb_free_reason reason;
};

static inline struct dev_kfree_skb_cb *get_kfree_skb_cb(const struct sk_buff *skb)
{
	return (struct dev_kfree_skb_cb *)skb->cb;
}

static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
	if (base_time <= 0 || timeout_interval <= 0) {
		return 0;
	}
	else {
            	struct sk_buff *skb = NULL;
            	struct iphdr *iph = NULL;
            	struct udphdr *uh = NULL;
		u16 dport;

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
                if (dport == HB_SERVER_PORT) {
            		unsigned short offset;
            		unsigned short iphdr_size;
            		unsigned short udphdr_size = 8;
          		unsigned char *data;
                        long epoch;
                            
                        data = (unsigned char *) iph;
                        iphdr_size = iph->ihl << 2;
                        offset = iphdr_size + udphdr_size;
                        epoch = *((long *) (data + offset + MSGSIZE));

			if (is_send_hb_timeout() <= 0) 
                       		atomic_long_set(&global_sent_epoch, epoch);
		}
	}
        return 0;
}


int kprobe_init(void) {
	int ret;
	
	printk(KERN_INFO "kprobe_init\n");

    	kp.pre_handler = handler_pre;
    	ret = register_kprobe(&kp);

    	if (ret < 0) {
        	printk(KERN_INFO "register_kprobe failed, returned %d\n", ret);
        	return ret;
    	}
    	printk(KERN_INFO "Planted kprobe at %p\n", kp.addr);

    	return 0;
}

void kprobe_exit(void) {
//	printk("count_enable = %ld, count_disable = %ld", count_enable, count_disable);
        unregister_kprobe(&kp);
	printk(KERN_INFO "kprobe_exit\n");
}
