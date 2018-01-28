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

uint32_t parseIPV4string(char* ipAddress) {
        unsigned int ipbytes[4];
        sscanf(ipAddress, "%uhh.%uhh.%uhh.%uhh", &ipbytes[3], &ipbytes[2], &ipbytes[1], &ipbytes[0]);
        return ipbytes[0] | ipbytes[1] << 8 | ipbytes[2] << 16 | ipbytes[3] << 24;
}

int init_module() {
        uint32_t local_ip = parseIPV4string("10.10.1.2");
        uint32_t remote_ip = parseIPV4string("10.10.1.11");            
        //uint32_t bytes = 0x549f35147440;

        np_t.name = "LRNG";
        strlcpy(np_t.dev_name, "em2", IFNAMSIZ);
        //np_t.local_ip.ip = htonl((unsigned long int) 0x0a000066);
        //np_t.remote_ip.ip = htonl((unsigned long int) 0x0a000065);
        np_t.local_ip.ip = htonl((unsigned long int) local_ip);
        np_t.remote_ip.ip = htonl((unsigned long int) remote_ip);
        np_t.local_port = 6665;
        np_t.remote_port = 33001;
        
        // mac addr of d4:6d:50:cf:c5:f2
        np_t.remote_mac[0] = 0xd4;
        np_t.remote_mac[1] = 0x6d;
        np_t.remote_mac[2] = 0x50;
        np_t.remote_mac[3] = 0xcf;
        np_t.remote_mac[4] = 0xc5;
        np_t.remote_mac[5] = 0xf2; 

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
