#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h> 
#include <linux/moduleparam.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <net/ip.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");

static struct nf_hook_ops nfho; // struct holding set of hook function options
static __be32 hb_sender = 0x0d00000a; // ip: 10.0.0.13

static int interval_ms=200;
module_param(interval_ms, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(interval_ms, "An integer");
EXPORT_SYMBOL(interval_ms);

static ktime_t last_time;
static ktime_t current_time;

/*hook function*/
unsigned int hook_func(const struct nf_hook_ops *ops, struct sk_buff *skb, 
        const struct net_device *in, const struct net_device *out, 
        int (*okfn)(struct sk_buff *))
{
        const struct iphdr *iph; 
        s64 interval_ns;
        long long INTERVAL_NS = interval_ms*1000*1000; 

//        printk("[msx] interval_ms in hbtracker is %d now\n", interval_ms);
        iph = ip_hdr(skb);
//        if(hb_sender == iph->saddr) { // check if ip comes from the one we are waiting for.
                printk(KERN_INFO "[msx] heartbeat comes from %pI4\n", &iph->saddr);
                current_time = ktime_get();
                interval_ns = ktime_to_ns(ktime_sub(current_time, last_time));
                last_time = current_time;
         //       printk(KERN_INFO "[msx] interval between two hbs is %lld ns, latency is %lld ns\n", (long long) interval_ns, (long long)interval_ns-INTERVAL_NS);
                
  //      }
        return NF_ACCEPT; 
}

int init_module()
{
        nfho.hook = hook_func;        
        nfho.hooknum = 0; // nfho.hooknum = NF_IP_POST_ROUTING;  
        nfho.pf = PF_INET; // IPV4 packets
        nfho.priority = NF_IP_PRI_FIRST; // set to highest priority over all other hook functions
        nf_register_hook(&nfho);  
        printk("interval_ms = %d\n", interval_ms);
        return 0; 
}

void cleanup_module()
{
        nf_unregister_hook(&nfho);   
}
