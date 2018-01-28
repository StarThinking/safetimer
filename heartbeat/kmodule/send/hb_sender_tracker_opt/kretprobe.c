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

//static long local_sent_epoch[64];
DEFINE_PER_CPU(long, local_sent_epoch);
static atomic_long_t global_sent_epoch;
//static long global_sent_epoch;
//static DEFINE_SPINLOCK(global_sent_epoch_lock);

static inline long get_local_sent_epoch(void);
static inline void update_local_sent_epoch(void);
static inline void set_global_sent_epoch(long epoch);
static inline long now(void);

//long prev_sent_epoch;
atomic_long_t enable;
//DEFINE_SPINLOCK(enable_lock);

extern long base_time;
extern long timeout_interval;

void public_set_global_sent_epoch(long epoch) {
	set_global_sent_epoch(epoch);
}

void reset_sent_epoch(void) {
	//for (i=0; i<64; i++)
	this_cpu_write(local_sent_epoch, 0);
        atomic_long_set(&global_sent_epoch, 0);
}

static inline long get_local_sent_epoch(void) {
        return this_cpu_read(local_sent_epoch);
}

static inline void update_local_sent_epoch(void) {
	long latest_epoch = atomic_long_read(&global_sent_epoch);
	this_cpu_write(local_sent_epoch, latest_epoch);
}

static inline void set_global_sent_epoch(long epoch) {
	atomic_long_set(&global_sent_epoch, epoch);  
}

static inline long now(void) {
        //return ktime_to_ms(ktime_get());
        return ktime_to_ms(ktime_get_real());
}

/* If sent_epoch = 0, return -1 to indicate not prepared. */
static inline long get_send_hb_timeout(int flag) {
        long sent_epoch, recv_timeout;
        
	if (flag == 0) {
        	/* get_local_sent_epoch() NOT involves spinlock contention. */
       		if ((sent_epoch = get_local_sent_epoch()) == 0)
                	return -1;
        } else if (flag == 1) {
		/* update_local_sent_epoch() involves spinlock contention. */
		update_local_sent_epoch();
                if ((sent_epoch = get_local_sent_epoch()) == 0)
                        return -1;
	}

        recv_timeout = epoch_to_time(sent_epoch + 1);
        
        return recv_timeout - get_max_transfer_delay() - get_max_clock_deviation();
}

long get_send_block_timeout(int flag) {
        return get_send_hb_timeout(flag) + get_max_transfer_delay();
}

int block_send(int flag) {
        long now_t = now();
        long send_block_timeout = get_send_block_timeout(flag);

        if (!prepared())
                return 0;

        if (send_block_timeout < 0)
                return 0;
        
    //    printk("now is %ld, send_block_timeout is %ld\n", now_t, send_block_timeout);
        return now_t < send_block_timeout ? 0 : 1;
}

static inline long is_send_hb_timeout(int flag) {
        long send_timeout = 0;
        long now_t = 0;
        
        //if (!prepared())
        //        return 0;

        send_timeout = get_send_hb_timeout(flag);

        /* Not prepared. */
        if (send_timeout < 0)
                return 0;
        
        now_t = now();
        
        return now_t - send_timeout;
}

//static long count_enable;
//static long count_disable;

static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {

	if (atomic_long_read(&enable) == 0 || base_time <= 0 || timeout_interval <= 0) {
//		count_disable ++;
		return 0;
	}
	else {
            	struct sk_buff *skb = NULL;
            	struct iphdr *iph = NULL;
            	struct udphdr *uh = NULL;
		u16 dport;

//		count_enable ++;
        	
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
	    		int global_flag;
          		unsigned char *data;
                        long epoch;
                        long exceeding_time;
                            
                        data = (unsigned char *) iph;
                        iphdr_size = iph->ihl << 2;
                        offset = iphdr_size + udphdr_size;
                        epoch = *((long *) (data + offset + MSGSIZE));
			global_flag = 0;

			if ((exceeding_time = is_send_hb_timeout(global_flag)) > 0) {
				global_flag = 1;
				exceeding_time = is_send_hb_timeout(global_flag);
			}

                        if (exceeding_time <= 0) {
                                //epoch_inc = epoch - prev_sent_epoch;
                                //inc_global_sent_epoch(epoch_inc);
				atomic_long_set(&enable, 0);
				atomic_long_set(&global_sent_epoch, epoch);
				//spin_lock_bh(&enable_lock);
				//enable = 0;
				//spin_unlock_bh(&enable_lock);
//                                printk("heartbeat sending completes and set sent_epoch as %ld, int %ld, prev %ld\n", 
 //                                       epoch, epoch_inc, prev_sent_epoch);
                        } 
                       
                }
	}
        return 0;
}

struct kretprobe my_kretprobe = {
	.entry_handler		= entry_handler,
	// Probe up to 20 instances concurrently. 
	.maxactive		= 50,
};

int kretprobe_init(void) {
	int ret;

	my_kretprobe.kp.symbol_name = "napi_consume_skb";
	ret = register_kretprobe(&my_kretprobe);
	if (ret < 0) {
		printk(KERN_INFO "register_kretprobe failed, returned %d\n", ret);
		return -1;
	}
	printk(KERN_INFO "Planted return probe at %s: %p\n",
			my_kretprobe.kp.symbol_name, my_kretprobe.kp.addr);
//	disable_kretprobe(&my_kretprobe);      
        return 0;
}

void kretprobe_exit(void) {
//	printk("count_enable = %ld, count_disable = %ld", count_enable, count_disable);
        unregister_kretprobe(&my_kretprobe);
	printk(KERN_INFO "kretprobe at %p unregistered\n",
			my_kretprobe.kp.addr);
}
