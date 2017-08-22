#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/hardirq.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <linux/netdevice.h>

extern struct net init_net;

static int __init fetch_driver_nstats_init(void) {
        struct net_device* dev;
        const struct net_device_ops *ops;
        struct rtnl_link_stats64 storage;
        struct rtnl_link_stats64 *storage_p = &storage;
        long skb_dev_drop;
            
        printk(KERN_INFO "fetch_driver_nstats init\n");
        
        if((dev = dev_get_by_name(&init_net, "em1")) == NULL)
                return 0;

        skb_dev_drop = atomic_long_read(&dev->rx_dropped);
        printk("[fetch_driver_nstats] skb_dev_drop = %lu\n", skb_dev_drop);
        
        ops = dev->netdev_ops;

        if (ops != NULL && ops->ndo_get_stats64) {
                memset(storage_p, 0, sizeof(*storage_p));
                ops->ndo_get_stats64(dev, storage_p);
                if(storage_p != NULL)
                        printk("[fetch_driver_nstats] driver rx_dropped = %llu, rx_packets = %llu\n", 
                                storage_p->rx_dropped, storage_p->rx_packets);
        }

        return 0;
}

static void __exit fetch_driver_nstats_exit(void) {
        printk(KERN_INFO "fetch_driver_nstats exit\n");
}

module_init(fetch_driver_nstats_init);
module_exit(fetch_driver_nstats_exit);
