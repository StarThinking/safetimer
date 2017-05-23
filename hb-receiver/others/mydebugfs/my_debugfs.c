#include <linux/init.h>
#include <linux/module.h>
#include <linux/debugfs.h> /* this is for DebugFS libraries */
#include <linux/fs.h>   

MODULE_LICENSE("GPL");

#define len 200

struct dentry *dirret,*fileret;
char ker_buf[len];
int filevalue;

/* read file operation */
static ssize_t myreader(struct file *fp, char __user *user_buffer,
                                size_t count, loff_t *position)
{
     return simple_read_from_buffer(user_buffer, count, position, ker_buf, len);
}
 
/* write file operation */
static ssize_t mywriter(struct file *fp, const char __user *user_buffer,
                                size_t count, loff_t *position)
{
        if(count > len )
                return -EINVAL;
  
        return simple_write_to_buffer(ker_buf, len, position, user_buffer, count);
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
    fileret = debugfs_create_file("text", 0644, dirret, &filevalue, &fops_debug);
 
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
