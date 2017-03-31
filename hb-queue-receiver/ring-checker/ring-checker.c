#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h> 
#include <linux/moduleparam.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <uapi/linux/netfilter_ipv4.h>
#include <net/ip.h>
#include <linux/ktime.h>
#include <linux/skbuff.h>

MODULE_LICENSE("GPL");

static int irq;
module_param(irq, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(irq, "An integer");

static int port;
module_param(port, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(port, "An integer");

static struct nf_hook_ops nfho0;
//nfho4; // struct holding set of hook function options

/*hook function*/
unsigned int hook_func(const struct nf_hook_ops *ops, struct sk_buff *skb, 
        const struct net_device *in, const struct net_device *out, 
        int (*okfn)(struct sk_buff *)) {
        struct iphdr *ip;
        //struct udphdr *udp;
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

        /*if(ip->protocol == IPPROTO_UDP) {
            udp = (struct udphdr *) skb_transport_header(skb);
            sport = (unsigned int) ntohs(udp->source);
            dport = (unsigned int) ntohs(udp->dest);
        } */
        if(ip->protocol == IPPROTO_TCP) { 
                tcp = (struct tcphdr *) skb_transport_header(skb);
                sport = (unsigned int) ntohs(tcp->source);
                dport = (unsigned int) ntohs(tcp->dest);

                if(dport == port) {
                        printk(KERN_DEBUG "[msx] hooknum %u, %pI4:%u --> %pI4:%u, irq_vec = %u, prot = %u, in = %s, out = %s\n", ops->hooknum, &saddr, sport, &daddr, dport, irq_vec, proto, in_name, out_name);

                        if(irq_vec != irq) {
                                printk("[msx] tcp, dport = %u but irq_vec = %u not %u, drop this packet", dport, irq_vec, irq);
                                return NF_DROP;
                        }
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
        
        /* NF_IP_POST_ROUTING */
        /*
        nfho4.hook = hook_func;        
        nfho4.hooknum = 4; 
        nfho4.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho4);  
        // set to highest priority over all other hook functions
        // nfho4.priority = NF_IP_PRI_FIRST;
        */

        return 0; 
}

void cleanup_module() {
        nf_unregister_hook(&nfho0);   
//        nf_unregister_hook(&nfho4);   
}
