#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h> 
#include <linux/moduleparam.h>

#include <net/ip.h>
#include <linux/inet.h>
#include <linux/netpoll.h>

#define MESSAGE_SIZE 8

MODULE_LICENSE("GPL");

static struct netpoll* np = NULL;
static struct netpoll np_t;

/*uint32_t parseIPV4string(char* ipAddress) {
        char ipbytes[4];
        sscanf(ipAddress, "%uhh.%uhh.%uhh.%uhh", &ipbytes[3], &ipbytes[2], &ipbytes[1], &ipbytes[0]);
        return ipbytes[0] | ipbytes[1] << 8 | ipbytes[2] << 16 | ipbytes[3] << 24;
}*/

int init_module() {
        //uint32_t local_ip = parseIPV4string("10.0.0.12");
        //uint32_t remote_ip = parseIPV4string("10.0.0.11");            

        np_t.name = "LRNG";
        strlcpy(np_t.dev_name, "em1", IFNAMSIZ);
        np_t.local_ip.ip = htonl((unsigned long int) 0x0a00000c);
        np_t.remote_ip.ip = htonl((unsigned long int) 0x0a00000c);
        np_t.local_port = 6665;
        np_t.remote_port = 5001;
        memset(np_t.remote_mac, 0xff, ETH_ALEN);
        netpoll_print_options(&np_t);
        netpoll_setup(&np_t);
        np = &np_t;

        return 0; 
}

void cleanup_module() {
        char message[MESSAGE_SIZE];
        sprintf(message,"%d\n",123);
        //int len = strlen(message);
        netpoll_send_udp(np,message,MESSAGE_SIZE);
}
