#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>

#include <linux/skbuff.h>
#include <net/ip.h>

MODULE_LICENSE("GPL");

#define HB_PORT 5001
#define LOCAL_PORT 5002

struct my_data {
        unsigned int dport;
};

static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
        struct my_data *data = (struct my_data *)ri->data;
        unsigned int dport = data->dport;

        printk("dport = %u\n", dport);
        
        if(dport == HB_PORT || dport == LOCAL_PORT) {
            printk("[hb_rps_kretprobe] change retval to -1 to disable rps for heartbeats and self msgs\n");
            regs->ax = -1;
        }

        return 0;
}

static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
        struct sk_buff *skb;
        struct iphdr *iph;
        unsigned int dport = 0;
        struct my_data *data = (struct my_data *)ri->data;
        
        data->dport = dport;

        if(regs != NULL) {
            skb = (struct sk_buff *) regs->si;
            if(skb != NULL) {
                iph = ip_hdr(skb);
                if(iph != NULL) {
                    if(iph->protocol == IPPROTO_UDP) {
                        struct udphdr *udph = (struct udphdr *) skb_transport_header(skb);
                        if(udph != NULL) {
                            dport = (size_t) ntohs(udph->dest);
                            data->dport = dport;
                        }
                    } else if(iph->protocol == IPPROTO_TCP) {
                        struct tcphdr *tcph = (struct tcphdr *) skb_transport_header(skb);
                        if(tcph != NULL) { 
                            dport = (size_t) ntohs(tcph->dest);
                            data->dport = dport;
                        }
                    } 
                }
            }
        }

        return 0;
}

static struct kretprobe my_kretprobe = {
        .entry_handler          = entry_handler,
	.handler		= ret_handler,
        .data_size              = sizeof(struct my_data),
	/* Probe up to 20 instances concurrently. */
	.maxactive		= 20,
};

static int __init kretprobe_init(void)
{
	int ret;

	my_kretprobe.kp.symbol_name = "get_rps_cpu";
	ret = register_kretprobe(&my_kretprobe);
	if (ret < 0) {
		printk(KERN_INFO "register_kretprobe failed, returned %d\n",
				ret);
		return -1;
	}
	printk(KERN_INFO "Planted return probe at %s: %p\n",
			my_kretprobe.kp.symbol_name, my_kretprobe.kp.addr);
        
        return 0;
}

static void __exit kretprobe_exit(void)
{
	unregister_kretprobe(&my_kretprobe);
	printk(KERN_INFO "kretprobe at %p unregistered\n",
			my_kretprobe.kp.addr);

	/* nmissed > 0 suggests that maxactive was set too low. */
	printk(KERN_INFO "Missed probing %d instances of %s\n",
		my_kretprobe.nmissed, my_kretprobe.kp.symbol_name);
}

module_init(kretprobe_init)
module_exit(kretprobe_exit)
MODULE_LICENSE("GPL");
