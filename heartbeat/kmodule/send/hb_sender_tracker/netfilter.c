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

MODULE_LICENSE("GPL");

static struct nf_hook_ops nfho0;

#define send_check_tlb_max 100

u64 send_check_tlb[send_check_tlb_max];
EXPORT_SYMBOL(send_check_tlb);

int send_check_tlb_index = 0;
EXPORT_SYMBOL(send_check_tlb_index);

DEFINE_SPINLOCK(send_check_tlb_lock);
EXPORT_SYMBOL(send_check_tlb_lock);

static uint64_t start, end;
static unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
static int index = 0;

static unsigned int hook_func(const struct nf_hook_ops *ops, struct sk_buff *skb, 
        const struct net_device *in, const struct net_device *out, 
        int (*okfn)(struct sk_buff *)) {
/*      
        asm volatile ("CPUID\n\t"
                "RDTSC\n\t"
                "mov %%edx, %0\n\t"
                "mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low)::
                "%rax", "%rbx", "%rcx", "%rdx");
*/
        struct iphdr *iph;
        struct tcphdr *th;
        struct udphdr *uh;
        unsigned int saddr, daddr;
        unsigned int sport = 0, dport = 0;
        long exceeding_time = 0;

        iph = (struct iphdr *) skb_network_header(skb);
        saddr = (unsigned int) iph->saddr;
        daddr = (unsigned int) iph->daddr;
        
        if(iph->protocol == IPPROTO_TCP) { 
                th = (struct tcphdr *) skb_transport_header(skb);
                sport = (size_t) ntohs(th->source);
                dport = (size_t) ntohs(th->dest);
        } else if(iph->protocol == IPPROTO_UDP){
                uh = (struct udphdr *) skb_transport_header(skb);
                sport = (size_t) ntohs(uh->source);
                dport = (size_t) ntohs(uh->dest);
        } 
        
//        if(iph->protocol == IPPROTO_TCP || iph->protocol == IPPROTO_UDP)
//                printk(KERN_DEBUG "[msx] POST_ROUTING %pI4:%u --> %pI4:%u, skb->data_len = %u\n", &saddr, sport, &daddr, dport, skb->data_len);
        
        // 0 if not timeout; positive if timeout, the value means the exceeding time beyond timeout
        exceeding_time = timeout();
        
        if(exceeding_time > 0) { // if timeout

                // filter that decides if packets should be dropped
                if(iph->protocol == IPPROTO_UDP && dport == 5001) {
                        //printk(KERN_DEBUG "[msx] UDP send to dport 5001 is disabled as it exceeds timeout %ld by %ld ms !\n", get_send_timeout(), exceeding_time);       
                        return NF_DROP;
                } 
        } 
/*
        asm volatile ("CPUID\n\t"
                "RDTSC\n\t"
                "mov %%edx, %0\n\t"
                "mov %%eax, %1\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                "%rax", "%rbx", "%rcx", "%rdx");

        start = ( ((uint64_t)cycles_high << 32) | cycles_low );
        end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
         
        spin_lock(&send_check_tlb_lock);

        index = send_check_tlb_index % send_check_tlb_max;
        send_check_tlb[index] = (end - start);
        send_check_tlb_index++;
        
        spin_unlock(&send_check_tlb_lock);
*/      
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
