#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "drop.h"
#define BUFFERSIZE 20

static unsigned long nic_dropped();
unsigned long nic_dropped_pkt_current;
static unsigned long nic_dropped_pkt_prev;

static unsigned long dev_dropped();
unsigned long dev_dropped_pkt_current;
static unsigned long dev_dropped_pkt_prev;

static unsigned long queue_dropped();
unsigned long queue_dropped_pkt_current;
static unsigned long queue_dropped_pkt_prev;

int init_drop() {
        FILE *fp;
        int ret = 0;
        char buffer[BUFFERSIZE];
        char *ptr;
      
        /* Init nic drop. */
        fp = fopen("/sys/kernel/debug/nic_drop_reader/dropped_packets", "r");
        if (fp == NULL) {
                perror("fopen nic_drop_reader");
                fclose(fp);
                ret = -1;
                goto error;
        }

        memset(buffer, '\0', BUFFERSIZE);
        fgets(buffer, BUFFERSIZE, fp);
        nic_dropped_pkt_prev = (unsigned long) strtol(buffer, &ptr, 10);
        fclose(fp);
        
        /* Init dev drop. */
        fp = fopen("/sys/kernel/debug/dev_drop_reader/dropped_packets", "r"); 
        if (fp == NULL) {
                perror("fopen dev_drop_reader");
                fclose(fp);
                ret = -1;
                goto error;
        }

        memset(buffer, '\0', BUFFERSIZE);
        fgets(buffer, BUFFERSIZE, fp);
        dev_dropped_pkt_prev = (unsigned long) strtol(buffer, &ptr, 10);
        fclose(fp);
        
        /* Init queue drop. */
        queue_dropped_pkt_prev = 0;

        printf("Drop has been initialized successfully.\n");

 error:
        return ret;
}

void destroy_drop() {
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
        FILE *fp;
        char buffer[BUFFERSIZE];
        unsigned long diff = 0;
        char *ptr;
        
        fp = fopen("/sys/kernel/debug/nic_drop_reader/dropped_packets", "r+");
        if (fp == NULL) {
                perror("fopen nic_drop_reader");
                fclose(fp);
                return 0;
        }

        memset(buffer, '\0', BUFFERSIZE);
        fgets(buffer, BUFFERSIZE, fp);
        nic_dropped_pkt_current = (unsigned long) strtol(buffer, &ptr, 10);
        fclose(fp);

        if ((diff = nic_dropped_pkt_current - nic_dropped_pkt_prev) > 0) {
                nic_dropped_pkt_prev = nic_dropped_pkt_current;
        }

        if (diff > 0)
                printf("Drop: nic dropped_pkt = %lu.\n", diff);

        return diff;
}

static unsigned long dev_dropped() {
        FILE *fp;
        char buffer[BUFFERSIZE];
        unsigned long diff = 0;
        char *ptr;

        fp = fopen("/sys/kernel/debug/dev_drop_reader/dropped_packets", "r+"); 
        if (fp == NULL) {
                perror("fopen dev_drop_reader");
                fclose(fp);
                return 0;
        }
        
        memset(buffer, '\0', BUFFERSIZE);
        fgets(buffer, BUFFERSIZE, fp);
        dev_dropped_pkt_current = (unsigned long) strtol(buffer, &ptr, 10);
        fclose(fp);

        if ((diff = dev_dropped_pkt_current - dev_dropped_pkt_prev) > 0) {
                dev_dropped_pkt_prev = dev_dropped_pkt_current;
        }

        if (diff > 0)
                printf("Drop: dev dropped_pkt = %lu.\n", diff);

        return diff;
}

static unsigned long queue_dropped() {
        unsigned long diff = 0;

        if ((diff = queue_dropped_pkt_current - queue_dropped_pkt_prev) > 0) {
                queue_dropped_pkt_prev = queue_dropped_pkt_current;
        }
        
        if (diff > 0)
                printf("Drop: queue dropped_pkt = %lu.\n", diff);

        return diff;
}

/*int main () {
        init_drop();
        nic_dropped();
        destroy_drop();
        return 0;
}*/
