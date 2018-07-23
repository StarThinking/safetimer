#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/ktime.h>
#include <net/sock.h>

#include <linux/kprobes.h>

MODULE_LICENSE("GPL");

static int count;
static int default_sk_rcvbuf;

struct my_data {
    struct sock *sk_saved;
};

static char func_name[NAME_MAX] = "udp_queue_rcv_skb";

static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
        struct my_data *data = (struct my_data *)ri->data;
        struct sock *sk = (struct sock*) regs->di;

        data->sk_saved = sk;

        if (count++ % 20 == 0) {
                default_sk_rcvbuf = sk->sk_rcvbuf;
                sk->sk_rcvbuf = 0;
        }

        printk(KERN_INFO "entry_handler: sk->sk_rcvbuf = %d\n", sk->sk_rcvbuf);
        
        return 0;
}

static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
    struct my_data *data = (struct my_data *)ri->data;
    struct sock *sk = data->sk_saved;

    sk->sk_rcvbuf = default_sk_rcvbuf;

    if (sk != NULL) {
            printk("ret_handler: sk_rcvbuf = %d\n", sk->sk_rcvbuf);
    }
    
    return 0;
}

static struct kretprobe my_kretprobe = {
        .entry_handler          = entry_handler,
        .handler                = ret_handler,
        .data_size              = sizeof(struct my_data),
        /* Probe up to 20 instances concurrently. */
        .maxactive              = 20,
};

static int __init drop_udp_queue_rcv_skb_init(void) {
    int ret;

    printk(KERN_INFO "drop_udp_queue_rcv_skb_init\n");
  
    my_kretprobe.kp.symbol_name = func_name;
    ret = register_kretprobe(&my_kretprobe);

    if (ret < 0) {
        printk(KERN_INFO "register_kretprobe failed, returned %d\n", ret);
        return -1;
    }
    
    printk(KERN_INFO "Planted return probe at %s: %p\n",
            my_kretprobe.kp.symbol_name, my_kretprobe.kp.addr);

    return 0;
}
 
static void __exit drop_udp_queue_rcv_skb_exit(void) {
    unregister_kretprobe(&my_kretprobe);
    printk(KERN_INFO "drop_udp_queue_rcv_skb_exit\n");
}
 
module_init(drop_udp_queue_rcv_skb_init);
module_exit(drop_udp_queue_rcv_skb_exit);
