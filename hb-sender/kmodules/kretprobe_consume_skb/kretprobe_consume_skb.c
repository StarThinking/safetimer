/*
 * kretprobe_example.c
 *
 * Here's a sample kernel module showing the use of return probes to
 * report the return value and total time taken for probed function
 * to run.
 *
 * usage: insmod kretprobe_example.ko func=<func_name>
 *
 * If no func_name is specified, do_fork is instrumented
 *
 * For more information on theory of operation of kretprobes, see
 * Documentation/kprobes.txt
 *
 * Build and insert the kernel module as done in the kprobe example.
 * You will see the trace data in /var/log/messages and on the console
 * whenever the probed function returns. (Some messages may be suppressed
 * if syslogd is configured to eliminate duplicate messages.)
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>

#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/fs.h>

#include <linux/skbuff.h>
#include <net/ip.h>

//#define len 64

//static struct dentry *dirret, *fileret;
//static unsigned long hb_completion_time = 0;

static char func_name[NAME_MAX] = "consume_skb";
module_param_string(func, func_name, NAME_MAX, S_IRUGO);
MODULE_PARM_DESC(func, "Function to kretprobe; this module will report the"
			" function's execution time");

/* read file operation */
/*static ssize_t myreader(struct file *fp, char __user *user_buffer,
                                size_t count, loff_t *position)
{
//        printk(KERN_INFO "hb_completion_time = %lu, now = %lu\n", hb_completion_time, jiffies);
        
        return simple_read_from_buffer(user_buffer, count, position, &hb_completion_time, len);
}*/

/*static const struct file_operations fops_debug = {
        .read = myreader
};
*/

/* Here we use the entry_hanlder to timestamp function entry */
static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
        struct sk_buff **skb_p = NULL;
        struct sk_buff *skb = NULL;
        struct iphdr *iph = NULL;
        struct udphdr *uh = NULL;
        u16 dport = 0;

        if(regs == NULL)
                return 0;

        skb_p = (struct sk_buff **) (regs->ax);
        if(skb_p == NULL)
                return 0;
        else
            skb = *skb_p;

        iph = ip_hdr(skb);
        if(iph == NULL)
                return 0;

        if(iph->protocol != IPPROTO_UDP)
                return 0;

        uh = udp_hdr(skb);
        if(uh == NULL)
                return 0;
        
        dport = ntohs(uh->dest);
        if(dport != 5001)
                return 0;
        
        unsigned long last_event_jiffies = jiffies;
        
        return 0;
}

/*
 * Return-probe handler: Log the return value and duration. Duration may turn
 * out to be zero consistently, depending upon the granularity of time
 * accounting on the platform.
 */
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
/*        struct sk_buff *skb = NULL;
        struct iphdr *iph = NULL;
        struct udphdr *uh = NULL;
        u16 dport = 0;
        //__be16 hb_dport = (__be16) htons(5001); 
        const uint16_t hb_port = 5001;
        //int p = 5001;
        //__be16 hb_port = htons(p);
        
        if((skb = (struct sk_buff *) ri->data) != NULL) {
                if((iph = (struct iphdr *) skb_network_header(skb)) != NULL
                                        && iph->protocol == IPPROTO_UDP) {
                            uh = udp_hdr(skb);
                            if(uh != NULL) {
                                    dport = ntohs(uh->dest);
                                   // if(dport != 5001)
                                     //       return 0;
                            }
                }
        }

        //u16 dport = ntohs(uh->dest);
        //if(uh->dest != hb_port)
          //      return 0;
        
        //if(ri->ret_addr == 0xffffffffa00780b5 && dport) {
        if(dport == (u16)hb_port) {
                hb_completion_time = jiffies;
        }
*/
        /*ADDR_SIZE = sizeof(ri->ret_addr);
        int count = sizeof(ri->ret_addr);
        if(!count || (count + log_size) > len)
                return 0;
        memcpy(kbuffer + log_size, &(ri->ret_addr), count);
        log_size += count;
        */

        return 0;
}

static struct kretprobe my_kretprobe = {
	.handler		= ret_handler,
	.entry_handler		= entry_handler,
	/* Probe up to 20 instances concurrently. */
	.maxactive		= 20,
};

static int __init kretprobe_init(void)
{
	int ret;

//        dirret = debugfs_create_dir("dell", NULL);
//        fileret = debugfs_create_file("text", 0644, dirret, NULL, &fops_debug);

	my_kretprobe.kp.symbol_name = func_name;
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
//        debugfs_remove_recursive(dirret);

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
