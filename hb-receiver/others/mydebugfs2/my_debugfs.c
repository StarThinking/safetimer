#include <linux/init.h>
#include <linux/module.h>
#include <linux/debugfs.h> /* this is for DebugFS libraries */
#include <linux/fs.h>   

#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

#define len 8

struct dentry *dirret,*fileret;
char ker_buf[len];
int log_size = 0;

/* read file operation */
static ssize_t myreader(struct file *fp, char __user *user_buffer,
                                size_t count, loff_t *position)
{
     printk("read: count = %zu, position = %lld\n", count, *position);
     return simple_read_from_buffer(user_buffer, count, position, ker_buf, len);
}
 
/* write file operation */
static ssize_t mywriter(struct file *fp, const char __user *user_buffer,
                                size_t count, loff_t *ppos)
{
        printk("write: count = %zu, position = %lld\n", count, *ppos);
        if((count + log_size) > len)
                return -EINVAL;
  
        loff_t pos = *ppos;
        size_t res;

        if (pos < 0)
                return -EINVAL;
        if (pos >= len || !count || (pos + log_size + count) > len)
                return 0;
        if (count > len - pos -log_size)
                count = len - pos - log_size;
        res = copy_from_user(ker_buf + log_size + pos, user_buffer, count);
        if (res == count)
                return -EFAULT;
        count -= res;
        log_size += count;
        *ppos = pos + log_size + count;
        return count;
}
 
static const struct file_operations fops_debug = {
        .read = myreader,
        .write = mywriter,
};
 
static int __init init_debug(void)
{
    /* create a directory by the name dell in /sys/kernel/debugfs */
    dirret = debugfs_create_dir("dell", NULL);
      
    /* create a file in the above directory
    This requires read and write file operations */
    fileret = debugfs_create_file("text", 0644, dirret, NULL, &fops_debug);
 
    return 0;
}
module_init(init_debug);
 
static void __exit exit_debug(void)
{
    /* removing the directory recursively which
    in turn cleans all the file */
    debugfs_remove_recursive(dirret);
}
module_exit(exit_debug);
