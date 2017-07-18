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

static struct nf_hook_ops nfho0, nfho1, nfho2, nfho3, nfho4;

/*hook function*/
unsigned int hook_func0(const struct nf_hook_ops *ops, struct sk_buff *skb, 
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
                printk(KERN_DEBUG "[msx] pre %pI4 --> %pI4, irq_vec = %u\n", 
                        &saddr, &daddr, irq_vec);

        return NF_ACCEPT; 
}

unsigned int hook_func1(const struct nf_hook_ops *ops, struct sk_buff *skb, 
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
                printk(KERN_DEBUG "[msx] post %pI4 --> %pI4, irq_vec = %u\n", 
                        &saddr, &daddr, irq_vec);

        return NF_ACCEPT; 
}

unsigned int hook_func2(const struct nf_hook_ops *ops, struct sk_buff *skb, 
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
                printk(KERN_DEBUG "[msx] input %pI4 --> %pI4, irq_vec = %u\n", 
                        &saddr, &daddr, irq_vec);

        return NF_ACCEPT; 
}

unsigned int hook_func3(const struct nf_hook_ops *ops, struct sk_buff *skb, 
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
                printk(KERN_DEBUG "[msx] output %pI4 --> %pI4, irq_vec = %u\n", 
                        &saddr, &daddr, irq_vec);

        return NF_ACCEPT; 
}

unsigned int hook_func4(const struct nf_hook_ops *ops, struct sk_buff *skb, 
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
                printk(KERN_DEBUG "[msx] forward %pI4 --> %pI4, irq_vec = %u\n", 
                        &saddr, &daddr, irq_vec);

        return NF_ACCEPT; 
}

int init_module() {
        nfho0.hook = hook_func0;        
        nfho0.hooknum = NF_INET_PRE_ROUTING; 
        nfho0.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho0);  
        
        nfho1.hook = hook_func1;        
        nfho1.hooknum = NF_INET_POST_ROUTING; 
        nfho1.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho1);  
        
        nfho2.hook = hook_func2;        
        nfho2.hooknum = NF_INET_LOCAL_IN; 
        nfho2.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho2);  
        
        nfho3.hook = hook_func3;        
        nfho3.hooknum = NF_INET_LOCAL_OUT; 
        nfho3.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho3);  
        
        nfho4.hook = hook_func4;        
        nfho4.hooknum = NF_INET_FORWARD; 
        nfho4.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho4);  
        
        return 0; 
}

void cleanup_module() {
        nf_unregister_hook(&nfho0);   
        nf_unregister_hook(&nfho1);   
        nf_unregister_hook(&nfho2);   
        nf_unregister_hook(&nfho3);   
        nf_unregister_hook(&nfho4);   
}
