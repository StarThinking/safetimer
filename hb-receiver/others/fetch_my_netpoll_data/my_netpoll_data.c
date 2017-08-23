#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h> 
#include <linux/moduleparam.h>

#include <net/ip.h>
#include <linux/inet.h>
#include <linux/netpoll.h>

MODULE_LICENSE("GPL");

struct netpoll_trace_data { 
        unsigned int sport, dport, saddr, daddr;
        int status;
};

extern struct netpoll_trace_data my_netpoll_data;

int init_module() {
        printk("%pI4:%u --> %pI4:%u\n", &my_netpoll_data.saddr, my_netpoll_data.sport, &my_netpoll_data.daddr, my_netpoll_data.dport);
        printk("my_netpoll_data.status = %d\n", my_netpoll_data.status);
        
        return 0; 
}

void cleanup_module() {
}
