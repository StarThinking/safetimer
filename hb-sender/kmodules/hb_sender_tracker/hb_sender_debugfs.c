#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/debugfs.h>
#include <linux/fs.h>

#include "hb_sender_debugfs.h"
#include "hb_sender_kretprobe.h"

MODULE_LICENSE("GPL");

long base_time = 0;
long timeout_interval = 0;

static struct dentry *dir_entry;
static struct dentry *hb_compl_entry;
static struct dentry*base_time_entry;
static struct dentry *timeout_interval_entry;

static char base_time_str[BUFFERSIZE];
static char timeout_interval_str[BUFFERSIZE];

static ssize_t hb_send_compl_time_read(struct file *fp, char __user *user_buffer,
                                size_t count, loff_t *position) {
        char str[BUFFERSIZE] = "";

        printk(KERN_INFO "hb_send_compl_time = %ld, udp_send_time = %ld\n", hb_send_compl_time, udp_send_time);
        sprintf(str, "%ld", hb_send_compl_time);
        return simple_read_from_buffer(user_buffer, count, position, str, BUFFERSIZE);
}

static ssize_t base_time_read(struct file *fp, char __user *user_buffer,
                                    size_t count, loff_t *position) {
        printk(KERN_INFO "base_time = %ld\n", base_time);
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
        printk(KERN_INFO "timeout_interval = %ld\n", timeout_interval);
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

static const struct file_operations hb_send_compl_time_fops = {
        .read = hb_send_compl_time_read
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
        hb_compl_entry = debugfs_create_file("hb_compl", 0644, dir_entry, NULL, &hb_send_compl_time_fops);
        base_time_entry = debugfs_create_file("base_time", 0644, dir_entry, NULL, &base_time_fops);
        timeout_interval_entry = debugfs_create_file("timeout_interval", 0644, dir_entry, NULL, &timeout_interval_fops);
        
	printk(KERN_INFO "debugfs inited\n");
}

void debugfs_exit(void) {
        debugfs_remove_recursive(dir_entry);
    
	printk(KERN_INFO "debugfs_exited\n");
}
