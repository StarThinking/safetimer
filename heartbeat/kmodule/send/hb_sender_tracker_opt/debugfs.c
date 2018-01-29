#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/spinlock_types.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/kprobes.h>

#include <linux/netpoll.h>
#include <net/ip.h>
#include <linux/inet.h>

#include "debugfs.h"
#include "kprobe.h"

//MODULE_LICENSE("GPL");

#define MSGSIZE sizeof(long)

long base_time = 0;
long timeout_interval = 0;

static struct dentry *dir_entry;
static struct dentry *sent_epoch_entry;
static struct dentry *base_time_entry;
static struct dentry *timeout_interval_entry;
static struct dentry *clear;

static char sent_epoch_str[BUFFERSIZE];
static char base_time_str[BUFFERSIZE];
static char timeout_interval_str[BUFFERSIZE];

//static struct netpoll np_t;

static int set_to_zero(void *data, u64 value) {
	reset_sent_epoch();
	
	memset(sent_epoch_str, 0, BUFFERSIZE);   
        memset(base_time_str, 0, BUFFERSIZE);   
        memset(timeout_interval_str, 0, BUFFERSIZE);   

        timeout_interval = 0;
        base_time = 0;
        return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(clear_fops, NULL, set_to_zero, "%llu\n");

static ssize_t sent_epoch_read(struct file *fp, char __user *user_buffer,
                                size_t count, loff_t *position) {
        return simple_read_from_buffer(user_buffer, count, position, sent_epoch_str, BUFFERSIZE);
}

static ssize_t sent_epoch_write(struct file *fp, const char __user *user_buffer,
                                     size_t count, loff_t *position) {
        ssize_t ret;
        int success;
        long epoch;

        if(count > BUFFERSIZE)
                return -EINVAL;

        ret =  simple_write_to_buffer(sent_epoch_str, BUFFERSIZE, position, user_buffer, count);
       
        success = kstrtol(sent_epoch_str, 10, &epoch);
        public_set_global_sent_epoch(epoch);
        
        if(success != 0)
                printk(KERN_INFO "sent_epoch_str conversion failed!\n");

        return ret;
}

static ssize_t base_time_read(struct file *fp, char __user *user_buffer,
                                    size_t count, loff_t *position) {
        return simple_read_from_buffer(user_buffer, count, position, base_time_str, BUFFERSIZE);
}

static ssize_t base_time_write(struct file *fp, const char __user *user_buffer,
                                     size_t count, loff_t *position) {
        ssize_t ret;

        if(count > BUFFERSIZE)
                return -EINVAL;	
        ret =  simple_write_to_buffer(base_time_str, BUFFERSIZE, position, user_buffer, count);
        if(kstrtol(base_time_str, 10, &base_time) != 0)
                printk(KERN_INFO "base_time_str conversion failed!\n");

        return ret;
}

static ssize_t timeout_interval_read(struct file *fp, char __user *user_buffer,
                                    size_t count, loff_t *position) {
        return simple_read_from_buffer(user_buffer, count, position, timeout_interval_str, BUFFERSIZE);
}

static ssize_t timeout_interval_write(struct file *fp, const char __user *user_buffer,
                                     size_t count, loff_t *position) {
        ssize_t ret;

        if(count > BUFFERSIZE)
                return -EINVAL;

        ret =  simple_write_to_buffer(timeout_interval_str, BUFFERSIZE, position, user_buffer, count);
        if(kstrtol(timeout_interval_str, 10, &timeout_interval) != 0)
                printk(KERN_INFO "timeout_interval_str conversion failed!\n");

        return ret;
}

static const struct file_operations sent_epoch_fops = {
        .read = sent_epoch_read,
        .write = sent_epoch_write
};
 
static const struct file_operations timeout_interval_fops = {
        .read = timeout_interval_read,
        .write = timeout_interval_write
};
 
static const struct file_operations base_time_fops = {
        .read = base_time_read,
        .write = base_time_write
};

void debugfs_init(void) {
	printk(KERN_INFO "debugfs init\n");

        dir_entry = debugfs_create_dir("hb_sender_tracker", NULL);
        sent_epoch_entry = debugfs_create_file("sent_epoch", 0644, dir_entry, NULL, &sent_epoch_fops);
        base_time_entry = debugfs_create_file("base_time", 0644, dir_entry, NULL, &base_time_fops);
        timeout_interval_entry = debugfs_create_file("timeout_interval", 0644, dir_entry, NULL, &timeout_interval_fops);
        clear = debugfs_create_file("clear", 0222, dir_entry, NULL, &clear_fops);

	/*np_t.name = "LRNG";
        strlcpy(np_t.dev_name, "enp6s0f0", IFNAMSIZ);
        //np_t.local_ip.ip = htonl((unsigned long int) 0x0a000066);
        //np_t.remote_ip.ip = htonl((unsigned long int) 0x0a000065);
        np_t.local_ip.ip = htonl((unsigned long int) 0x0a0a0102);
        np_t.remote_ip.ip = htonl((unsigned long int) 0x0a0a0101);
        np_t.local_port = 6665;
        np_t.remote_port = 5001;

        // mac addr of d4:6d:50:cf:c5:f2
        np_t.remote_mac[0] = 0x90;
        np_t.remote_mac[1] = 0xe2;
        np_t.remote_mac[2] = 0xba;
        np_t.remote_mac[3] = 0x83;
        np_t.remote_mac[4] = 0xca;
        np_t.remote_mac[5] = 0x5c;

        memset(np_t.remote_mac, 0xff, ETH_ALEN);
        netpoll_print_options(&np_t);
        netpoll_setup(&np_t);

	printk(KERN_INFO "netpoll setup\n");*/
	printk(KERN_INFO "debugfs inited\n");
}

void debugfs_exit(void) {
        debugfs_remove_recursive(dir_entry);
    
	printk(KERN_INFO "debugfs_exited\n");
}
