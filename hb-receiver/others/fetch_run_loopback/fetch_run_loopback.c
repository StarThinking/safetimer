#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h> 
#include <linux/moduleparam.h>

#include <net/ip.h>
#include <linux/inet.h>
#include <linux/netpoll.h>

MODULE_LICENSE("GPL");

extern int tg3_run_loopback_runs;
extern int tg3_run_loopback_ret;

int init_module() {
        printk("tg3_run_loopback_runs = %d, tg3_run_loopback_ret = %d\n", 
                tg3_run_loopback_runs, tg3_run_loopback_ret);
        
        return 0; 
}

void cleanup_module() {
}
