#include <linux/kernel.h>
#include <linux/module.h>

#include "debugfs.h"
//#include "kretprobe.h"
#include "kprobe.h"
#include "netfilter.h"

MODULE_LICENSE("GPL");

static int __init tracker_init(void) {
        printk("tracker_init\n");
        debugfs_init();
        kprobe_init();
        netfilter_init();
        return 0;
}

static void __exit tracker_exit(void) {
        netfilter_exit();
        kprobe_exit();
//	disable_intercept();
        debugfs_exit();
}

module_init(tracker_init)
module_exit(tracker_exit)
