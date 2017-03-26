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

/*hook function*/
unsigned int hook_func(const struct nf_hook_ops *ops, struct sk_buff *skb, 
        const struct net_device *in, const struct net_device *out, 
        int (*okfn)(struct sk_buff *))
{
        struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb);
        struct udphdr *udp_header;
        struct tcphdr *tcp_header;

        unsigned int src_ip = (unsigned int)ip_header->saddr;
        unsigned int dest_ip = (unsigned int)ip_header->daddr;
        unsigned int src_port = 0, dest_port = 0;

        unsigned int ring_id = skb->napi_id;

        if(ip_header->protocol == 17) { //  udp
            udp_header = (struct udphdr *)skb_transport_header(skb);
            src_port = (unsigned int)ntohs(udp_header->source);
            dest_port = (unsigned int)ntohs(udp_header->dest);
        } else if(ip_header->protocol == 6) { // tcp
            tcp_header = (struct tcphdr *)skb_transport_header(skb);
            src_port = (unsigned int)ntohs(tcp_header->source);
            dest_port = (unsigned int)ntohs(tcp_header->dest);
        }

        printk(KERN_DEBUG "[msx] src = %pI4 : %u, dst = %pI4 : %u, ring = %u, prot = %u\n", &src_ip, src_port, &dest_ip, dest_port, ring_id, ip_header->protocol);
        
        return NF_ACCEPT; 
}

int init_module()
{
        nfho.hook = hook_func;        
        nfho.hooknum = 0; // nfho.hooknum = NF_IP_POST_ROUTING;  
        nfho.pf = PF_INET; // IPV4 packets
        nfho.priority = NF_IP_PRI_FIRST; // set to highest priority over all other hook functions
        nf_register_hook(&nfho);  
        return 0; 
}

void cleanup_module()
{
        nf_unregister_hook(&nfho);   
}
