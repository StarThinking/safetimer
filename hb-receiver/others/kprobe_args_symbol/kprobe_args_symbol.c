#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>

#include <linux/kprobes.h>

int foobar2(int arg1, int arg2, int arg3)
{
    printk(KERN_INFO "foobar called\n");
    
    return arg3;
}
EXPORT_SYMBOL(foobar2);

static int __init kprobe_args_init(void)
{    
    return 0;
}
 
static void __exit kprobe_args_exit(void)
{
}
 
module_init(kprobe_args_init);
module_exit(kprobe_args_exit);
 
MODULE_LICENSE("GPL");
