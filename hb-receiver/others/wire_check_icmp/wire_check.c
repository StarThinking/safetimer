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
unsigned int hook_pre(const struct nf_hook_ops *ops, struct sk_buff *skb, 
        const struct net_device *in, const struct net_device *out, 
        int (*okfn)(struct sk_buff *)) {
        struct iphdr *ip;
        unsigned int saddr, daddr;
        unsigned int irq_vec;
        char str[INET_ADDRSTRLEN];
        const char *in_name, *out_name;

        in_name = in->name;
        out_name = out->name;

        ip = (struct iphdr *) skb_network_header(skb);
        saddr = (unsigned int) ip->saddr;
        daddr = (unsigned int) ip->daddr;

        irq_vec = skb->napi_id; // this value is modified to be irq in driver

        sprintf(str, "%pI4", &saddr);

        if(ip->protocol ==  IPPROTO_ICMP)
                printk(KERN_DEBUG "[msx] pre %pI4 --> %pI4, irq_vec = %u, in = %s, out = %s, hash = %u\n", 
                        &saddr, &daddr, irq_vec, in_name, out_name, skb_get_hash(skb));

        return NF_ACCEPT; 
}

unsigned int hook_post(const struct nf_hook_ops *ops, struct sk_buff *skb, 
        const struct net_device *in, const struct net_device *out, 
        int (*okfn)(struct sk_buff *)) {
        struct iphdr *ip;
        unsigned int saddr, daddr;
        unsigned int irq_vec;
        char str[INET_ADDRSTRLEN];
        const char *in_name, *out_name;

        in_name = in->name;
        out_name = out->name;

        ip = (struct iphdr *) skb_network_header(skb);
        saddr = (unsigned int) ip->saddr;
        daddr = (unsigned int) ip->daddr;

        irq_vec = skb->napi_id; // this value is modified to be irq in driver

        sprintf(str, "%pI4", &saddr);

        if(ip->protocol ==  IPPROTO_ICMP)
                printk(KERN_DEBUG "[msx] post %pI4 --> %pI4, irq_vec = %u, in = %s, out = %s\n", 
                        &saddr, &daddr, irq_vec, in_name, out_name);

        return NF_ACCEPT; 
}

unsigned int hook_in(const struct nf_hook_ops *ops, struct sk_buff *skb, 
        const struct net_device *in, const struct net_device *out, 
        int (*okfn)(struct sk_buff *)) {
        struct iphdr *ip;
        unsigned int saddr, daddr;
        unsigned int irq_vec;
        char str[INET_ADDRSTRLEN];
        const char *in_name, *out_name;

        in_name = in->name;
        out_name = out->name;

        ip = (struct iphdr *) skb_network_header(skb);
        saddr = (unsigned int) ip->saddr;
        daddr = (unsigned int) ip->daddr;

        irq_vec = skb->napi_id; // this value is modified to be irq in driver

        sprintf(str, "%pI4", &saddr);

        if(ip->protocol ==  IPPROTO_ICMP)
                printk(KERN_DEBUG "[msx] in %pI4 --> %pI4, irq_vec = %u, in = %s, out = %s\n", 
                        &saddr, &daddr, irq_vec, in_name, out_name);

        return NF_ACCEPT; 
}

unsigned int hook_out(const struct nf_hook_ops *ops, struct sk_buff *skb, 
        const struct net_device *in, const struct net_device *out, 
        int (*okfn)(struct sk_buff *)) {
        struct iphdr *ip;
        unsigned int saddr, daddr;
        unsigned int irq_vec;
        char str[INET_ADDRSTRLEN];
        const char *in_name, *out_name;

        in_name = in->name;
        out_name = out->name;

        ip = (struct iphdr *) skb_network_header(skb);
        saddr = (unsigned int) ip->saddr;
        daddr = (unsigned int) ip->daddr;

        irq_vec = skb->napi_id; // this value is modified to be irq in driver

        sprintf(str, "%pI4", &saddr);

        if(ip->protocol ==  IPPROTO_ICMP)
                printk(KERN_DEBUG "[msx] out %pI4 --> %pI4, irq_vec = %u, in = %s, out = %s\n", 
                        &saddr, &daddr, irq_vec, in_name, out_name);

        return NF_ACCEPT; 
}

unsigned int hook_forward(const struct nf_hook_ops *ops, struct sk_buff *skb, 
        const struct net_device *in, const struct net_device *out, 
        int (*okfn)(struct sk_buff *)) {
        struct iphdr *ip;
        unsigned int saddr, daddr;
        unsigned int irq_vec;
        char str[INET_ADDRSTRLEN];
        const char *in_name, *out_name;

        in_name = in->name;
        out_name = out->name;

        ip = (struct iphdr *) skb_network_header(skb);
        saddr = (unsigned int) ip->saddr;
        daddr = (unsigned int) ip->daddr;

        irq_vec = skb->napi_id; // this value is modified to be irq in driver

        sprintf(str, "%pI4", &saddr);

        if(ip->protocol ==  IPPROTO_ICMP)
                printk(KERN_DEBUG "[msx] forward %pI4 --> %pI4, irq_vec = %u, in = %s, out = %s\n", 
                        &saddr, &daddr, irq_vec, in_name, out_name);

        return NF_ACCEPT; 
}

int init_module() {
        nfho0.hook = hook_pre;        
        nfho0.hooknum = NF_INET_PRE_ROUTING; 
        nfho0.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho0);  
        
        nfho1.hook = hook_post;        
        nfho1.hooknum = NF_INET_POST_ROUTING; 
        nfho1.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho1);  
        
        nfho2.hook = hook_in;        
        nfho2.hooknum = NF_INET_LOCAL_IN; 
        nfho2.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho2);  
        
        nfho3.hook = hook_out;        
        nfho3.hooknum = NF_INET_LOCAL_OUT; 
        nfho3.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho3);  
        
        nfho4.hook = hook_forward;        
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
