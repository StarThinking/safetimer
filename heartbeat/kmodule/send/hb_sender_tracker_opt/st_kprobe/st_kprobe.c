#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>

#include <linux/skbuff.h>
#include <net/ip.h>

#include "../../../../include/hb_common.h"

#define SIG_TEST 44 

MODULE_LICENSE("GPL");

extern long st_pid; // from st_debugfs.c

static struct siginfo info;
static struct task_struct *t;

static struct kprobe kp = {
    .symbol_name    = "napi_consume_skb", //__dev_kfree_skb_irq
};

static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
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
//        printk("handler_pre dport = %u\n", dport);
        if (dport == HB_SERVER_PORT) {
                int ret = send_sig_info(SIG_TEST, &info, t); 
                if (ret < 0) {
                        pr_err("error sending signal\n");
                }
                printk("signal sent!\n");
            	//unsigned short offset;
            	//unsigned short iphdr_size;
            	//unsigned short udphdr_size = 8;
          	//unsigned char *data;
                //long epoch;
                            
                //data = (unsigned char *) iph;
                //iphdr_size = iph->ihl << 2;
                //offset = iphdr_size + udphdr_size;
                //epoch = *((long *) (data + offset + MSGSIZE));
	}
        return 0;
}

static int __init st_kprobe_init(void) {
	int ret;
    
        if (st_pid == 0) {
                printk("st_pid == 0)\n");
                return 0;
        }
        
	printk(KERN_INFO "st_kprobe_init\n");
    	kp.pre_handler = handler_pre;
    	ret = register_kprobe(&kp);
    	if (ret < 0) {
        	printk(KERN_INFO "register_kprobe failed, returned %d\n", ret);
        	return ret;
    	}
    	printk(KERN_INFO "Planted kprobe at %p\n", kp.addr);

        memset(&info, 0, sizeof(struct siginfo));
        info.si_signo = SIG_TEST;
        info.si_code = SI_QUEUE;
        info.si_int = 1234;
        rcu_read_lock();
        t = pid_task(find_pid_ns((int)st_pid, &init_pid_ns), PIDTYPE_PID);
        if (t == NULL) {
                pr_err("no such pid\n");
                rcu_read_unlock();
                return -ENODEV;
        }
        rcu_read_unlock();
        printk("find_pid_ns() succeed\n");

    	return 0;
}

static void __exit st_kprobe_exit(void) {
        unregister_kprobe(&kp);
	printk(KERN_INFO "st_kprobe_exit\n");
}

module_init(st_kprobe_init);
module_exit(st_kprobe_exit);
