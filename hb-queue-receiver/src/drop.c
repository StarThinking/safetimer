#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/types.h>
#include <time.h>

#include "drop.h"

static FILE *fp;
static char line[256];
static char *token, *eptr;
static int line_counter = 0;
static int column_counter = 0;

// fetch UDP InErrors in /proc/net/snmp
static long fetch_udp_inerrors() {
        long udp_inerrors = -1;
        char *url = "/proc/net/snmp";
        char delim[2] = " :";
        
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
        printf("nfqueue_pid = %d\n", pid);
        long netlink_drops = -1;
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
//                        printf("column_counter = %d, token = %s\n", column_counter, token);
                        // compare the 3rd column of each which is the pid of netlink program
                        if(column_counter == 2) {
                                _pid = strtol(token, &eptr, 10);
//                                printf("_pid = %ld\n", _pid);
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
        printf("netlink_drops = %ld\n", netlink_drops);
        return netlink_drops;
}

static int fetch_kernel_drop_stats(struct kernel_drop_stats *stats) {
        long _udp_inerrors;
        long _netlink_drops;

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

void init_kernel_drop(struct kernel_drop_stats *stats) {
        if(fetch_kernel_drop_stats(stats) < 0)
                printf("\n\nfetch kernel drop stats error!\n\n");
}

int check_kernel_drop(struct kernel_drop_stats *last_stats) {
        struct kernel_drop_stats current_stats;
        long udp_errors_diff, netlink_drops_diff;

        if(fetch_kernel_drop_stats(&current_stats) < 0) {
                printf("\n\nfetch kernel drop stats error!\n\n");
                exit(1);
        }       

        udp_errors_diff = current_stats.udp_errors - last_stats->udp_errors;
        netlink_drops_diff = current_stats.netlink_drops - last_stats->netlink_drops;

        printf("\t[Drop] udp_errors_diff = %ld, netlink_drops_diff = %ld.\n", \
                udp_errors_diff, netlink_drops_diff);
        *last_stats = current_stats;
        
        return (udp_errors_diff > 0) || (netlink_drops_diff > 0); 
}

int main(int argc, char *argv[]) {
        pid_t pid = atoi(argv[1]);
        fetch_netlink_drops(pid);
        return 0;
}
