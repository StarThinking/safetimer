#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

static struct dentry *dir;
static struct dentry *base_time_file, *timeout_interval_file;

static u64 base_time = 0;
static u64 timeout_interval = 0;

// This is called when the module loads.
int init_module(void)
{
    dir = debugfs_create_dir("hb_receiver", 0);
    if (!dir) {
        printk(KERN_ALERT "failed to create /sys/kernel/debug/hb_receiver\n");
        return -1;
    }

    base_time_file = debugfs_create_u64("base_time", 0666, dir, &base_time);
    timeout_interval_file = debugfs_create_u64("timeout_interval", 0666, dir, &timeout_interval);
    if (!base_time_file || !timeout_interval_file) {
        printk(KERN_ALERT "debugfs_example1: failed to create /sys/kernel/debug/base_time_file or timeout_interval_file\n");
        return -1;
    }

    return 0;
}

// This is called when the module is removed.
void cleanup_module(void)
{
    debugfs_remove_recursive(dir);
}
