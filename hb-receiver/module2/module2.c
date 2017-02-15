#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h> 
#include <linux/moduleparam.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <net/ip.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");

extern int interval_ms;

int init_module()
{
        interval_ms = 300;
        printk("interval_ms = %d\n", interval_ms);
        return 0; 
}

void cleanup_module()
{
    interval_ms = 200;
}
