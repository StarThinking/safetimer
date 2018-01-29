#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h> 
#include <linux/moduleparam.h>

#include <net/ip.h>
#include <linux/inet.h>
#include <linux/netpoll.h>

#define MESSAGE_SIZE 8

MODULE_LICENSE("GPL");

static struct netpoll np_t;


int init_module() {
        np_t.name = "LRNG";
        strlcpy(np_t.dev_name, "enp6s0f0", IFNAMSIZ);
        //np_t.local_ip.ip = htonl((unsigned long int) 0x0a000066);
        //np_t.remote_ip.ip = htonl((unsigned long int) 0x0a000065);
        np_t.local_ip.ip = htonl((unsigned long int) 0x0a0a0102);
        np_t.remote_ip.ip = htonl((unsigned long int) 0x0a0a0101);
        np_t.local_port = 6665;
        np_t.remote_port = 33001;
        
        // mac addr of d4:6d:50:cf:c5:f2
        np_t.remote_mac[0] = 0x90;
        np_t.remote_mac[1] = 0xe2;
        np_t.remote_mac[2] = 0xba;
        np_t.remote_mac[3] = 0x83;
        np_t.remote_mac[4] = 0xca;
        np_t.remote_mac[5] = 0x5c; 

        memset(np_t.remote_mac, 0xff, ETH_ALEN);
        netpoll_print_options(&np_t);
        netpoll_setup(&np_t);

        return 0; 
}

void cleanup_module() {
        char message[MESSAGE_SIZE] = "";
        long data = 1314;
        int count = 1;
        int i;

        sprintf(message, "%ld\n", data);
        //int len = strlen(message);
        for (i=0; i<count; i++)
            netpoll_send_udp(&np_t, message, MESSAGE_SIZE);
}
