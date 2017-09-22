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

#define HB_PORT 5001

static struct nf_hook_ops nfho;
static int count;

static unsigned int hook_func(const struct nf_hook_ops *ops, struct sk_buff *skb, 
        const struct net_device *in, const struct net_device *out, 
        int (*okfn)(struct sk_buff *)) {
        
        struct iphdr *iph;
        //unsigned int saddr, daddr;
        //char saddr_ip[INET_ADDRSTRLEN];
        //char daddr_ip[INET_ADDRSTRLEN];
        unsigned int sport, dport;
        //unsigned int irq_vec;
        struct ethhdr *hdr; 

        hdr = eth_hdr(skb);
        sport = dport = 0;
        //irq_vec = skb->napi_id;
        iph = (struct iphdr *) skb_network_header(skb);
        //saddr = (unsigned int) iph->saddr;
        //daddr = (unsigned int) iph->daddr;
        //sprintf(saddr_ip, "%pI4", &saddr);
        //sprintf(daddr_ip, "%pI4", &daddr);

        if (iph->protocol == IPPROTO_UDP) {
                struct udphdr *udph = (struct udphdr *) skb_transport_header(skb);
                sport = (size_t) ntohs(udph->source);
                dport = (size_t) ntohs(udph->dest);

                if (count % 10 == 0 && dport == HB_PORT) {
                        printk("heartbeat received and delay will be added afterwards.\n");
                        //usleep_range(2500, 3000);
                        mdelay(2100);
                }
                count ++;
        }  else
                return NF_ACCEPT;

        return NF_ACCEPT; 
}

int init_module() {
        nfho.hook = hook_func;        
        nfho.hooknum = NF_INET_PRE_ROUTING; 
        nfho.pf = PF_INET; // IPV4 packets
        nf_register_hook(&nfho);  

        return 0; 
}

void cleanup_module() {
        nf_unregister_hook(&nfho);   
}
