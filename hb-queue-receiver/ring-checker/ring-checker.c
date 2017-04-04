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

static int irq = 50;
module_param(irq, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(irq, "An integer");

/* dport for heartbear receiving */
static int port = 5001;
module_param(port, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(port, "An integer");

static struct nf_hook_ops nfho0;
static u8 myvalue;
static struct dentry *file, *dir;

/*hook function*/
unsigned int hook_func(const struct nf_hook_ops *ops, struct sk_buff *skb, 
        const struct net_device *in, const struct net_device *out, 
        int (*okfn)(struct sk_buff *)) {
        struct iphdr *ip;
        struct tcphdr *tcp;
        unsigned int sport, dport, saddr, daddr;
        unsigned int irq_vec, queue_mapping, proto;
        const char *in_name, *out_name;

        ip = (struct iphdr *) skb_network_header(skb);
        saddr = (unsigned int) ip->saddr;
        daddr = (unsigned int) ip->daddr;
        sport = dport = 0;

        irq_vec = skb->napi_id; // this value is modified to be irq in driver
        queue_mapping = skb->queue_mapping; // it's always 0
        proto = ip->protocol;
        in_name = in->name;
        out_name = out->name;

        if(ip->protocol == IPPROTO_TCP) { 
                tcp = (struct tcphdr *) skb_transport_header(skb);
                sport = (unsigned int) ntohs(tcp->source);
                dport = (unsigned int) ntohs(tcp->dest);

                if(dport == port) {
                        printk(KERN_DEBUG "[msx] hooknum %u, %pI4:%u --> %pI4:%u, irq_vec = %u, prot = %u, in = %s, out = %s\n", ops->hooknum, &saddr, sport, &daddr, dport, irq_vec, proto, in_name, out_name);
                }
        } 
        return NF_ACCEPT; 
}

int init_module() {
        /* NF_IP_PRE_ROUTING */
        nfho0.hook = hook_func;        
        nfho0.hooknum = 0; 
        nfho0.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho0);  
       
        dir = debugfs_create_dir("10.0.0.13", NULL);
        file = debugfs_create_u8("irq", 0644, dir, &myvalue);

        return 0; 
}

void cleanup_module() {
        nf_unregister_hook(&nfho0);   
        debugfs_remove(file);
        debugfs_remove(dir);
}
