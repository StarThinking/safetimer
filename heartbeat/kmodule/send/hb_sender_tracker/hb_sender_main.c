#include <linux/kernel.h>
#include <linux/module.h>

#include "hb_sender_debugfs.h"
#include "hb_sender_kretprobe.h"
#include "hb_sender_netfilter.h"

MODULE_LICENSE("GPL");

static int __init tracker_init(void) {
        printk("tracker_init\n");
        debugfs_init();
        kretprobe_init();
        netfilter_init();
        return 0;
}

static void __exit tracker_exit(void) {
        netfilter_exit();
        kretprobe_exit();
        debugfs_exit();
}

module_init(tracker_init)
module_exit(tracker_exit)
