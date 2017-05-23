#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/types.h>
#include <time.h>

#include "drop.h"

// fetch UDP InErrors in /proc/net/snmp
static long fetch_udp_inerrors() {
        long udp_inerrors = -1;
        FILE *fp;
        char line[256] = "";
        char *url = "/proc/net/snmp";
        // note that the last charactor of char array should be reserved for terminator
        char delim[3] = " :";
        char *token, *eptr;
        int line_counter = 0;
        int column_counter = 0;
        
        if((fp = fopen(url, "r")) == NULL) {
                printf("can't open file %s!\n", url);
                return udp_inerrors;
        }

        while(fgets(line, sizeof(line), fp)) {
                if(line_counter == 10) {
                        token = strtok(line, delim);
                        while(token != NULL) {
                                if(column_counter == 3) {
                                        udp_inerrors = strtol(token, &eptr, 10);
                                        break;
                                }
                                token = strtok(NULL, delim);
                                column_counter ++;
                        }
                        break;
                }
                line_counter ++;
        }
        
        fclose(fp);
        return udp_inerrors;
}

// fetch netlink drops by the process of nfqueuein /proc/net/netlink
static long fetch_netlink_drops(pid_t pid) {
        long netlink_drops = -1;
        FILE *fp;
        char line[256] = "";
        char *token, *eptr;
        int column_counter = 0;
        char *url = "/proc/net/netlink";
        char delim[2] = " ";
        long nfqueue_pid = (long) pid;
        long _pid;

        if((fp = fopen(url, "r")) == NULL) {
                printf("can't open file %s!\n", url);
                return netlink_drops;
        }

        while(fgets(line, sizeof(line), fp)) {
                token = strtok(line, delim);
                column_counter = 0;
                while(token != NULL) {
                        // compare the 3rd column of each which is the pid of netlink program
                        if(column_counter == 2) {
                                _pid = strtol(token, &eptr, 10);
                                if(_pid != nfqueue_pid) 
                                        break;
                        }

                        // column of drops
                        if(column_counter == 8) { 
                                netlink_drops = strtol(token, &eptr, 10);
                                goto found;
                        }

                        token = strtok(NULL, delim);
                        column_counter ++;
                }
        }

found:        
        fclose(fp);
        return netlink_drops;
}

static int fetch_all_kernel_drop_stats(struct kernel_drop_stats *stats) {
        long _udp_inerrors = 0;
        long _netlink_drops = 0;

        if((_udp_inerrors = fetch_udp_inerrors()) < 0) {
                printf("failed to fetch udp inerrors!\n");
                return -1;
        } 
        
        if((_netlink_drops = fetch_netlink_drops(stats->nfqueue_pid)) < 0) {
                printf("failed to fetch netlink drops!\n");
                return -1;
        }

        stats->udp_errors = _udp_inerrors;
        stats->netlink_drops = _netlink_drops;

        return 0;
}

int init_kernel_drop(struct kernel_drop_stats *stats) {
        if(fetch_all_kernel_drop_stats(stats) < 0) {
                printf("\n\nfetch kernel drop stats error!\n\n");
                return -1;
        }
        return 0;
}

int check_kernel_drop(struct kernel_drop_stats *last_stats) {
        struct kernel_drop_stats current_stats;
        long udp_errors_diff, netlink_drops_diff;

        current_stats.nfqueue_pid = last_stats->nfqueue_pid;
        if(fetch_all_kernel_drop_stats(&current_stats) < 0) {
                printf("\n\nfetch kernel drop stats error!\n\n");
                return -1;
        }       

        udp_errors_diff = current_stats.udp_errors - last_stats->udp_errors;
        netlink_drops_diff = current_stats.netlink_drops - last_stats->netlink_drops;

        printf("\t[Drop] udp_errors_diff = %ld, netlink_drops_diff = %ld.\n", 
                udp_errors_diff, netlink_drops_diff);
        *last_stats = current_stats;
        
        return (udp_errors_diff > 0) || (netlink_drops_diff > 0); 
}

// return 1 if drop, 0 if not, <0 if error
int check_nic_drop() {
        long nic_drop = -1;
        FILE *fp;
        char buffer[20] = "";
        char *url = "/sys/kernel/debug/nic-drop/pkt_dropped";
        char *ptr;

        if((fp = fopen(url, "r")) == NULL) {
                printf("can't open file %s!\n", url);
                goto error;
        }

        fscanf(fp, "%s", buffer);
        nic_drop = strtol(buffer, &ptr, 10);
       
        printf("\t[Drop] nic_drop = %ld.\n", nic_drop);
        fclose(fp);        
    
        return nic_drop > 0;


error:
        return nic_drop;
}

/*int main(int argc, char *argv[]) {
        check_nic_drop();   
        return 0;
}*/