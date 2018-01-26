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

#include "kretprobe.h"
#include "debugfs.h"
#include "../../../include/hb_common.h"

MODULE_LICENSE("GPL");

static struct nf_hook_ops nfho0;

//static unsigned int hook_func(const struct nf_hook_ops *ops, struct sk_buff *skb, 
//        const struct net_device *in, const struct net_device *out, 
//        int (*okfn)(struct sk_buff *)) {

static unsigned int hook_func(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
        /* If sending is valid. */
        if (!block_send()) 
                return NF_ACCEPT; 
        //if (1)
       
        else {
                struct iphdr *iph;
                struct tcphdr *th;
                struct udphdr *uh;
                unsigned int saddr, daddr;
                unsigned int sport = 0, dport = 0;

                iph = (struct iphdr *) skb_network_header(skb);
                saddr = (unsigned int) iph->saddr;
                daddr = (unsigned int) iph->daddr;
 
                /* Get the port. */
                if (iph->protocol == IPPROTO_TCP) { 
                        th = (struct tcphdr *) skb_transport_header(skb);
                        sport = (size_t) ntohs(th->source);
                        dport = (size_t) ntohs(th->dest);
                } else if (iph->protocol == IPPROTO_UDP){
                        uh = (struct udphdr *) skb_transport_header(skb);
                        sport = (size_t) ntohs(uh->source);
                        dport = (size_t) ntohs(uh->dest);
                } 
        
                //        if(iph->protocol == IPPROTO_TCP || iph->protocol == IPPROTO_UDP)
                //                printk(KERN_DEBUG "[msx] POST_ROUTING %pI4:%u --> %pI4:%u, skb->data_len = %u\n", &saddr, sport, &daddr, dport, skb->data_len);
        
                /* 
                * 0 if not timeout; positive if timeout 
                * The value means the exceeding time beyond timeout.
                */
        
                /* Rule matching. */
                if (iph->protocol == IPPROTO_UDP && dport == HB_SERVER_PORT) {
                        printk(KERN_DEBUG "[msx] UDP send to dport 5001 is disabled!\n");       
                        return NF_DROP;
                } 
        }
        return NF_ACCEPT; 
}

int netfilter_init(void) {
        nfho0.hook = hook_func;        
        nfho0.hooknum = NF_INET_POST_ROUTING; 
        nfho0.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho0);  

        return 0; 
}

void netfilter_exit(void) {
        nf_unregister_hook(&nfho0);   
}
