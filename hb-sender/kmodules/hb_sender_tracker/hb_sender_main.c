#include <linux/kernel.h>
#include <linux/module.h>

#include "hb_sender_debugfs.h"

MODULE_LICENSE("GPL");

static int __init tracker_init(void) {
        printk("tracker_init\n");
        debugfs_init();
        return 0;
}

static void __exit tracker_exit(void) {
        debugfs_exit();
}

module_init(tracker_init)
module_exit(tracker_exit)
