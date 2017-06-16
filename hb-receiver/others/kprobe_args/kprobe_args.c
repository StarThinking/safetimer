#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>

#include <linux/kprobes.h>

static struct kprobe kp;

static struct kprobe kp = {
    .symbol_name    = "foobar",
};

static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    printk(KERN_INFO "handler_pre: arg = %lu\n", regs->di);
    return 0;
}

static void handler_post(struct kprobe *p, struct pt_regs *regs,
                                          unsigned long flags)
{
    printk(KERN_INFO "handler_post\n");
}

extern void foobar(int);

static int __init kprobe_args_init(void)
{
    int ret;

    printk(KERN_INFO "kprobe_args_init\n");
    
//    kp.addr = (kprobe_opcode_t *) foobar;
    kp.pre_handler = handler_pre;
    kp.post_handler = handler_post;
    ret = register_kprobe(&kp);

    if (ret < 0) {
        printk(KERN_INFO "register_kprobe failed, returned %d\n", ret);
        return ret;
    }
    
    printk(KERN_INFO "Planted kprobe at %p\n", kp.addr);
    
    foobar(123);
    
    return 0;
}
 
static void __exit kprobe_args_exit(void)
{
    unregister_kprobe(&kp);
    printk(KERN_INFO "kprobe_args_exit\n");
}
 
module_init(kprobe_args_init);
module_exit(kprobe_args_exit);
 
MODULE_LICENSE("GPL");
