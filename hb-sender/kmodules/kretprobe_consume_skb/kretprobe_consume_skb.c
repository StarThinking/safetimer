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

#define BUFFERMAX 2048

static struct dentry *dirret, *fileret;
static char kbuffer[BUFFERMAX];
static int log_size = 0;

static char func_name[NAME_MAX] = "consume_skb";
module_param_string(func, func_name, NAME_MAX, S_IRUGO);
MODULE_PARM_DESC(func, "Function to kretprobe; this module will report the"
			" function's execution time");

/* Here we use the entry_hanlder to timestamp function entry */
static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	return 0;
}

/*
 * Return-probe handler: Log the return value and duration. Duration may turn
 * out to be zero consistently, depending upon the granularity of time
 * accounting on the platform.
 */
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	//int retval = regs_return_value(regs);
        struct sk_buff *skb = (struct sk_buff *) ri->data;
        char str[16] = "";
        unsigned int daddr;
        char *log_pos = NULL;
        struct iphdr *ip = NULL;

        if(skb) {
                ip = (struct iphdr *) skb_network_header(skb);
            if(ip) {
                    daddr = (unsigned int) ip->daddr;
                    sprintf(str, "%pI4", &daddr);

                    log_pos = (char *)(kbuffer + log_size);
                    memcpy(log_pos, str, strlen(str));
                    log_size += strlen(str);

	         //   printk(KERN_INFO "%s ret_addr = %p, daddr = %pI4\n", func_name, ri->ret_addr, &daddr);
            }
        }
	
        //printk(KERN_INFO "%s ret_addr = %p\n", func_name, ri->ret_addr);
        
        return 0;
}

static struct kretprobe my_kretprobe = {
	.handler		= ret_handler,
	.entry_handler		= entry_handler,
	/* Probe up to 20 instances concurrently. */
	.maxactive		= 20,
};

/* read file operation */
static ssize_t myreader(struct file *fp, char __user *user_buffer, size_t count, loff_t *position)
{
        return simple_read_from_buffer(user_buffer, count, position, kbuffer, log_size);
}

/* write file operation */
static ssize_t mywriter(struct file *fp, const char __user *user_buffer, size_t count, loff_t *position)
{
        if(count > BUFFERMAX)
                return -EINVAL;
        
        return simple_write_to_buffer(kbuffer, BUFFERMAX, position, user_buffer, count);
}

static const struct file_operations fops = {
        .read = myreader,
        .write = mywriter,
};

static int __init kretprobe_init(void)
{
	int ret;

	my_kretprobe.kp.symbol_name = func_name;
	ret = register_kretprobe(&my_kretprobe);
	if (ret < 0) {
		printk(KERN_INFO "register_kretprobe failed, returned %d\n",
				ret);
		return -1;
	}
	printk(KERN_INFO "Planted return probe at %s: %p\n",
			my_kretprobe.kp.symbol_name, my_kretprobe.kp.addr);

        dirret = debugfs_create_dir("kretprobe_consume_skb", NULL);
        fileret = debugfs_create_file("kretprobe.log", 0644, dirret, NULL, &fops);

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

        debugfs_remove_recursive(dirret);
}

module_init(kretprobe_init)
module_exit(kretprobe_exit)
MODULE_LICENSE("GPL");
