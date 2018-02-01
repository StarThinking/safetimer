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

MODULE_LICENSE("GPL");

#define BUFFERSIZE 20

static struct dentry *dir_entry;

long st_valid_time = 0;
EXPORT_SYMBOL(st_valid_time);
static char st_valid_time_str[BUFFERSIZE];
static struct dentry *st_valid_time_entry;

long st_pid = 0;
EXPORT_SYMBOL(st_pid);
static char st_pid_str[BUFFERSIZE];
static struct dentry *st_pid_entry;

static struct dentry *clear;

static ssize_t st_valid_time_read(struct file *fp, char __user *user_buffer,
                                            size_t count, loff_t *position) {
        return simple_read_from_buffer(user_buffer, count, position, st_valid_time_str, BUFFERSIZE);
}

static ssize_t st_valid_time_write(struct file *fp, const char __user *user_buffer,
                                     size_t count, loff_t *position) {
        ssize_t ret;
        int success;

        if(count > BUFFERSIZE)
                return -EINVAL;

        ret =  simple_write_to_buffer(st_valid_time_str, BUFFERSIZE, position, user_buffer, count);
        success = kstrtol(st_valid_time_str, 10, &st_valid_time);
        if(success != 0)
                printk(KERN_INFO "valid_time_str conversion failed!\n");

        return ret;
}

static const struct file_operations st_valid_time_fops = {
        .read = st_valid_time_read,
        .write = st_valid_time_write
};

static ssize_t st_pid_read(struct file *fp, char __user *user_buffer,
                                            size_t count, loff_t *position) {
        return simple_read_from_buffer(user_buffer, count, position, st_pid_str, BUFFERSIZE);
}

static ssize_t st_pid_write(struct file *fp, const char __user *user_buffer,
                                     size_t count, loff_t *position) {
        ssize_t ret;
        int success;

        if(count > BUFFERSIZE)
                return -EINVAL;

        ret = simple_write_to_buffer(st_pid_str, BUFFERSIZE, position, user_buffer, count);
        success = kstrtol(st_pid_str, 10, &st_pid);
        if(success != 0)
                printk(KERN_INFO "pid_str conversion failed!\n");

        return ret;
}

static const struct file_operations st_pid_fops = {
        .read = st_pid_read,
        .write = st_pid_write
};

static int set_to_zero(void *data, u64 value) {
        memset(st_valid_time_str, 0, BUFFERSIZE); 
        memset(st_pid_str, 0, BUFFERSIZE); 
        st_valid_time = 0;
        st_pid = 0;

        return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(clear_fops, NULL, set_to_zero, "%llu\n");

static int __init st_debugfs_init(void) {
	printk(KERN_INFO "st_debugfs_init\n");
        dir_entry = debugfs_create_dir("st_debugfs", NULL);
        st_valid_time_entry = debugfs_create_file("st_valid_time", 0644, dir_entry, NULL, &st_valid_time_fops);
        st_pid_entry = debugfs_create_file("st_pid", 0644, dir_entry, NULL, &st_pid_fops);
        clear = debugfs_create_file("clear", 0222, dir_entry, NULL, &clear_fops);           
        return 0;
}

static void __exit st_debugfs_exit(void) {
        debugfs_remove_recursive(dir_entry);
	printk(KERN_INFO "sy_debugfs_exited\n");
}

module_init(st_debugfs_init);
module_exit(st_debugfs_exit);
