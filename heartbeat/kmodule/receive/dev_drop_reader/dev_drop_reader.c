#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/hardirq.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <linux/netdevice.h>

#include "../../../include/hb_config.h"

#define BUFFERSIZE 20

MODULE_LICENSE("GPL");

static char dropped_packets_str[BUFFERSIZE];
static struct dentry *dir_entry;
static struct dentry *dropped_packets_entry;

static ssize_t dropped_packets_read(struct file *fp, char __user *user_buffer,
                                    size_t count, loff_t *position) { 
        struct net_device* dev;
        const struct net_device_ops *ops;
        struct rtnl_link_stats64 storage;
        long driver_dropped_pkts = 0;
        long kerneldev_dropped_pkts = 0;
        unsigned long dropped_packets = 0;

        if ((dev = dev_get_by_name(&init_net, BARRIER_SERVER_IF)) == NULL) {
                printk("dev_drop_reader: dev_get_by_name failed.\n");
                return 0;
        }

        kerneldev_dropped_pkts = atomic_long_read(&dev->rx_dropped);

        if ((ops = dev->netdev_ops) != NULL && ops->ndo_get_stats64) {
                struct rtnl_link_stats64 *storage_p = &storage;

                memset(storage_p, 0, sizeof(*storage_p));
                ops->ndo_get_stats64(dev, storage_p);

                if (storage_p != NULL)
                         driver_dropped_pkts = storage_p->rx_dropped;
        }

        dropped_packets = driver_dropped_pkts + kerneldev_dropped_pkts;

        snprintf(dropped_packets_str, 10, "%lu", dropped_packets);
        
        return simple_read_from_buffer(user_buffer, count, position, dropped_packets_str, BUFFERSIZE);
}

static ssize_t dropped_packets_write(struct file *fp, const char __user *user_buffer,
                                    size_t count, loff_t *position) {
        ssize_t ret;

        if (count > BUFFERSIZE)
                return -EINVAL;

        ret =  simple_write_to_buffer(dropped_packets_str, BUFFERSIZE, position, user_buffer, count);

        return ret;
}

static const struct file_operations dropped_packets_fops = {
        .read = dropped_packets_read,
        .write = dropped_packets_write
};

static int dev_drop_reader_init(void) {
        printk(KERN_INFO "dev_drop_reader init\n");

        dir_entry = debugfs_create_dir("dev_drop_reader", NULL);
        dropped_packets_entry = debugfs_create_file("dropped_packets", 0644, dir_entry, NULL, &dropped_packets_fops);

        return 0;
}

static void dev_drop_reader_exit(void) {
        debugfs_remove_recursive(dir_entry);

        printk(KERN_INFO "dev_drop_checker_exit\n");
}

module_init(dev_drop_reader_init)
module_exit(dev_drop_reader_exit)
