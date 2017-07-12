#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h> 
#include <linux/moduleparam.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <uapi/linux/netfilter_ipv4.h>
#include <net/ip.h>
#include <linux/inet.h>
#include <linux/ktime.h>
#include <linux/skbuff.h>

#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

static struct nf_hook_ops nfho0;

/*hook function*/
unsigned int hook_func(const struct nf_hook_ops *ops, struct sk_buff *skb, 
        const struct net_device *in, const struct net_device *out, 
        int (*okfn)(struct sk_buff *)) {
        struct iphdr *ip;
        unsigned int saddr, daddr;
        unsigned int irq_vec;
        char str[INET_ADDRSTRLEN];

        ip = (struct iphdr *) skb_network_header(skb);
        saddr = (unsigned int) ip->saddr;
        daddr = (unsigned int) ip->daddr;

        irq_vec = skb->napi_id; // this value is modified to be irq in driver

        sprintf(str, "%pI4", &saddr);

        if(ip->protocol ==  IPPROTO_ICMP)
                printk(KERN_DEBUG "[msx] %pI4 --> %pI4, irq_vec = %u\n", 
                        &saddr, &daddr, irq_vec);

        return NF_ACCEPT; 
}

int init_module() {
        nfho0.hook = hook_func;        
        nfho0.hooknum = NF_INET_PRE_ROUTING; 
        nfho0.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho0);  
        
        return 0; 
}

void cleanup_module() {
        nf_unregister_hook(&nfho0);   
}
