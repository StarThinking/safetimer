#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "hb_common.h"
#include "drop.h"

#define BUFFERSIZE 20

static unsigned long nic_incr_dropped();
static unsigned long dev_incr_dropped();
static unsigned long queue_incr_dropped();

unsigned long queue_dropped_pkt_current;
static unsigned long queue_dropped_pkt_prev;

int if_drop_happened() {
        int ret = 0;

        if (nic_incr_dropped() > 0)
                ret += NICDROP;

        if (dev_incr_dropped() > 0)
                ret += DEVDROP;

        if (queue_incr_dropped() > 0)
                ret += QUEUEDROP;

        return ret;
}

static unsigned long nic_incr_dropped() {
        FILE *fp;
        char buffer[BUFFERSIZE];
        char *ptr;
        unsigned long nic_incr;
        
        fp = fopen("/sys/kernel/debug/nic_drop_reader/dropped_packets", "r+");
        if (fp == NULL) {
                perror("fopen nic_drop_reader");
                fclose(fp);
                return 0;
        }

        memset(buffer, '\0', BUFFERSIZE);
        fgets(buffer, BUFFERSIZE, fp);
        nic_incr = (unsigned long) strtol(buffer, &ptr, 10);
        fclose(fp);

        if (nic_incr > 0) {
                printf("Drop: nic_incr = %lu.\n", nic_incr);
        }

        return nic_incr;
}

static unsigned long dev_incr_dropped() {
        FILE *fp;
        char buffer[BUFFERSIZE];
        char *ptr;
        unsigned long dev_incr;

        fp = fopen("/sys/kernel/debug/dev_drop_reader/dropped_packets", "r+"); 
        if (fp == NULL) {
                perror("fopen dev_drop_reader");
                fclose(fp);
                return 0;
        }
        
        memset(buffer, '\0', BUFFERSIZE);
        fgets(buffer, BUFFERSIZE, fp);
        dev_incr = (unsigned long) strtol(buffer, &ptr, 10);
        fclose(fp);

        if (dev_incr > 0) {
                recv_stats.dev_drop_pkts += dev_incr;
                printf("Drop: dev_incr = %lu.\n", dev_incr);
        }

        return dev_incr;
}


static unsigned long queue_incr_dropped() {
        unsigned long queue_incr;

        queue_dropped_pkt_prev = queue_dropped_pkt_current;
        queue_incr = queue_dropped_pkt_current - queue_dropped_pkt_prev;

        if (queue_incr > 0) {
                recv_stats.queue_drop_pkts += queue_incr;
                printf("Drop: queue_incr = %lu.\n", queue_incr);
        }

        return queue_incr;
}

/*int main () {
        init_drop();
        nic_dropped();
        destroy_drop();
        return 0;
}*/
