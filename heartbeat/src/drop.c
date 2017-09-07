#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "drop.h"
#define BUFFERSIZE 20

static unsigned long nic_dropped();

static unsigned long dev_dropped();
static FILE *dev_dropped_fp;
unsigned long dev_dropped_pkt_current;
static unsigned long dev_dropped_pkt_prev;

static unsigned long queue_dropped();
unsigned long queue_dropped_pkt_current;
static unsigned long queue_dropped_pkt_prev;

int init_drop() {
        int ret = 0;
        char buffer[BUFFERSIZE];
        char *ptr;
       
        /* Init dev drop. */
        dev_dropped_fp = fopen("/sys/kernel/debug/dev_drop_reader/dropped_packets", "r+"); 
        if (dev_dropped_fp == NULL) {
                perror("fopen dev_dropped_fp");
                fclose(dev_dropped_fp);
                ret = -1;
                goto error;
        }

        memset(buffer, '\0', BUFFERSIZE);
        fgets(buffer, BUFFERSIZE, dev_dropped_fp);
        dev_dropped_pkt_prev = (unsigned long) strtol(buffer, &ptr, 10);
        
        /* Init queue drop. */
        queue_dropped_pkt_prev = 0;

        printf("Drop has been initialized successfully.\n");

 error:
        return ret;
}

void destroy_drop() {
        fclose(dev_dropped_fp);

        printf("Drop has been destroyed.\n");
} 

int if_drop_happened() {
        int ret = 0;

        if (nic_dropped() > 0)
                ret += NICDROP;

        if (dev_dropped() > 0)
                ret += DEVDROP;

        if (queue_dropped() > 0)
                ret += QUEUEDROP;

        return ret;
}

static unsigned long nic_dropped() {
        return 0;
}

static unsigned long dev_dropped() {
        char buffer[BUFFERSIZE];
        unsigned long diff = 0;
        char *ptr;

        memset(buffer, '\0', BUFFERSIZE);
        fgets(buffer, BUFFERSIZE, dev_dropped_fp);
        dev_dropped_pkt_current = (unsigned long) strtol(buffer, &ptr, 10);

        if ((diff = dev_dropped_pkt_current - dev_dropped_pkt_prev) > 0) {
                dev_dropped_pkt_prev = dev_dropped_pkt_current;
        }

//        printf("dev_dropped diff is %lu\n", diff);

        return diff;
}

static unsigned long queue_dropped() {
        unsigned long diff = 0;

        if ((diff = queue_dropped_pkt_current - queue_dropped_pkt_prev) > 0) {
                queue_dropped_pkt_prev = queue_dropped_pkt_current;
        }
        
//        printf("queue_dropped diff is %lu\n", diff);

        return diff;
}

