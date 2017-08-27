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

MODULE_LICENSE("GPL");

static struct nf_hook_ops nfhos[5];
static const int hooknums[5] = {NF_INET_PRE_ROUTING, NF_INET_LOCAL_IN, NF_INET_FORWARD,NF_INET_LOCAL_OUT, NF_INET_POST_ROUTING};
static const char *hooknames[5] = {"PRE_ROUTING", "LOCAL_IN", "FORWARD", "LOCAL_OUT", "POST_ROUTING"};

static unsigned int hook_func(const struct nf_hook_ops *ops, struct sk_buff *skb, 
        const struct net_device *in, const struct net_device *out, 
        int (*okfn)(struct sk_buff *)) {
        
        struct iphdr *iph;
        unsigned int saddr, daddr;
        char saddr_ip[INET_ADDRSTRLEN];
        char daddr_ip[INET_ADDRSTRLEN];
        unsigned int sport, dport;
        unsigned int irq_vec;
        struct ethhdr *hdr; 

        hdr = eth_hdr(skb);
        sport = dport = 0;
        irq_vec = skb->napi_id;
        iph = (struct iphdr *) skb_network_header(skb);
        saddr = (unsigned int) iph->saddr;
        daddr = (unsigned int) iph->daddr;
        sprintf(saddr_ip, "%pI4", &saddr);
        sprintf(daddr_ip, "%pI4", &daddr);

        if(iph->protocol == IPPROTO_TCP) {
                struct tcphdr *tcph = (struct tcphdr *) skb_transport_header(skb);
                sport = (size_t) ntohs(tcph->source);
                dport = (size_t) ntohs(tcph->dest);
        } else if(iph->protocol == IPPROTO_UDP) {
                struct udphdr *udph = (struct udphdr *) skb_transport_header(skb);
                sport = (size_t) ntohs(udph->source);
                dport = (size_t) ntohs(udph->dest);
        } else if(iph->protocol == IPPROTO_ICMP) {
                ;
        } else
                return NF_ACCEPT;

       if(strcmp(saddr_ip, "10.0.0.1") != 0 && strcmp(daddr_ip, "10.0.0.1") != 0) {
                if(strcmp(saddr_ip, "0.0.0.0") != 0 && strcmp(daddr_ip, "0.0.0.0") != 0) {
                        //printk(KERN_DEBUG "[msx] %s (0x%pM) %pI4 : %u --> (0x%pM) %pI4 : %u via rx queue %u\n", 
                        //        hooknames[ops->hooknum], hdr->h_source, &saddr, sport, hdr->h_dest, &daddr, dport, irq_vec);
                        
                        printk(KERN_DEBUG "%s prot %u %pI4 : %u --> %pI4 : %u via rx queue %u\n", 
                                hooknames[ops->hooknum], iph->protocol, &saddr, sport, &daddr, dport, irq_vec);
                }
        }

        return NF_ACCEPT; 
}

int init_module() {
        int i = 0;
        for(; i<=4; i++) {
                nfhos[i].hook = hook_func;        
                nfhos[i].hooknum = hooknums[i]; 
                nfhos[i].pf = PF_INET; // IPV4 packets
                nf_register_hook(&nfhos[i]);  
        }

        return 0; 
}

void cleanup_module() {
        int i = 0;
        for(; i<=4; i++) {
                nf_unregister_hook(&nfhos[i]);   
        }
}
