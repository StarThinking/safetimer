#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h> 
#include <linux/moduleparam.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <net/ip.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");

extern bool hb_processed;

int init_module()
{
        if(hb_processed)
            printk("hb_processed\n");
        else
            printk("!hb_processed\n");
        return 0; 
}

void cleanup_module()
{
}
