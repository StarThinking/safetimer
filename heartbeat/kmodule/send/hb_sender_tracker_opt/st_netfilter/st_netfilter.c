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

#include "../../../../include/hb_common.h"

MODULE_LICENSE("GPL");

extern atomic_long_t st_valid_time;

static struct nf_hook_ops nfho0;

static unsigned int hook_func(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
        long _st_valid_time = atomic_long_read(&st_valid_time);	
	if (_st_valid_time == 0 || ktime_to_ms(ktime_get_boottime()) <= _st_valid_time) {
		return NF_ACCEPT;
	} else { 
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
        
                /* Rule matching. */
                if (iph->protocol == IPPROTO_UDP && dport == HB_SERVER_PORT) {
                    //    printk("ktime_to_ms(ktime_get_boottime()) = %lld\n", ktime_to_ms(ktime_get_boottime()));
                        printk(KERN_DEBUG "[msx] UDP send to dport 5001 is disabled!\n");       
                        return NF_DROP;
                } 
        }
        return NF_ACCEPT; 
}

static int __init st_netfilter_init(void) {
        nfho0.hook = hook_func;        
        nfho0.hooknum = NF_INET_POST_ROUTING; 
        nfho0.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho0);  

        return 0; 
}

static void __exit st_netfilter_exit(void) {
        nf_unregister_hook(&nfho0);   
}

module_init(st_netfilter_init);
module_exit(st_netfilter_exit);
