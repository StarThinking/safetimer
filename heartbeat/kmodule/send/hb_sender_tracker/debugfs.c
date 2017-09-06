#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/spinlock_types.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>

#include "debugfs.h"

MODULE_LICENSE("GPL");

long base_time = 0;
long timeout_interval = 0;
long sent_epoch = 0;

static DEFINE_SPINLOCK(sent_epoch_lock);

static struct dentry *dir_entry;
static struct dentry *sent_epoch_entry;
static struct dentry *base_time_entry;
static struct dentry *timeout_interval_entry;
static struct dentry *clear;

static char sent_epoch_str[BUFFERSIZE];
static char base_time_str[BUFFERSIZE];
static char timeout_interval_str[BUFFERSIZE];

static int set_to_zero(void *data, u64 value) {
        memset(sent_epoch_str, 0, BUFFERSIZE);   
        memset(base_time_str, 0, BUFFERSIZE);   
        memset(timeout_interval_str, 0, BUFFERSIZE);   

        spin_lock(&sent_epoch_lock);
        sent_epoch = 0;
        spin_unlock(&sent_epoch_lock);
        timeout_interval = 0;
        sent_epoch = 0;

        return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(clear_fops, NULL, set_to_zero, "%llu\n");

long get_sent_epoch(void) {
        long epoch;

        spin_lock_bh(&sent_epoch_lock);
        epoch = sent_epoch;
        spin_unlock_bh(&sent_epoch_lock);
        
        return epoch;
}

void set_sent_epoch(long epoch) {
        spin_lock_bh(&sent_epoch_lock);
        sent_epoch = epoch;
        spin_unlock_bh(&sent_epoch_lock);
        
        return;
}

static ssize_t sent_epoch_read(struct file *fp, char __user *user_buffer,
                                size_t count, loff_t *position) {
        return simple_read_from_buffer(user_buffer, count, position, sent_epoch_str, BUFFERSIZE);
}

static ssize_t sent_epoch_write(struct file *fp, const char __user *user_buffer,
                                     size_t count, loff_t *position) {
        ssize_t ret;
        int success;

        if(count > BUFFERSIZE)
                return -EINVAL;

        ret =  simple_write_to_buffer(sent_epoch_str, BUFFERSIZE, position, user_buffer, count);
        
        spin_lock(&sent_epoch_lock);
        success = kstrtol(sent_epoch_str, 10, &sent_epoch);
        spin_unlock(&sent_epoch_lock);
        
        if(success != 0)
                printk(KERN_INFO "sent_epoch_str conversion failed!\n");

        return ret;
}

static ssize_t base_time_read(struct file *fp, char __user *user_buffer,
                                    size_t count, loff_t *position) {
        //printk(KERN_INFO "base_time = %ld\n", base_time);
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
        //printk(KERN_INFO "timeout_interval = %ld\n", timeout_interval);
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
        
	printk(KERN_INFO "debugfs inited\n");
}

void debugfs_exit(void) {
        debugfs_remove_recursive(dir_entry);
    
	printk(KERN_INFO "debugfs_exited\n");
}
