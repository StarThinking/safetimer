#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/types.h>
#include <time.h>

#include "drop.h"

// fetch UDP InErrors in /proc/net/snmp
static long fetch_udp_inerrors() {
        FILE *fp;
        char *url = "/proc/net/snmp";
        char line[256];
        char delim[2] = " :";
        char *token, *eptr;
        int line_counter = 0;
        int field_counter = 0;
        long udp_inerrors = -1;
        
        if((fp = fopen(url, "r")) == NULL) {
                printf("can't open file %s!\n", url);
                return udp_inerrors;
        }

        while(fgets(line, sizeof(line), fp)) {
                if(line_counter == 10) {
                        token = strtok(line, delim);
                        while(token != NULL) {
                                if(field_counter == 3) {
                                        udp_inerrors = strtol(token, &eptr, 10);
                                        break;
                                }
                                token = strtok(NULL, delim);
                                field_counter ++;
                        }
                        break;
                }
                line_counter ++;
        }
        
        fclose(fp);

        return udp_inerrors;
}

static int fetch_kernel_drop_stats(struct kernel_drop_stats *stats) {
        long _udp_inerrors;
        if((_udp_inerrors = fetch_udp_inerrors()) < 0) {
                printf("failed to fetch udp inerrors!\n");
                return -1;
        } else {
                stats->udp_errors = _udp_inerrors;
        }

        return 0;
}

void init_kernel_drop(struct kernel_drop_stats *stats) {
        if(fetch_kernel_drop_stats(stats) < 0)
                printf("\n\nfetch kernel drop stats error!\n\n");
}

int check_kernel_drop(struct kernel_drop_stats *last_stats) {
        struct kernel_drop_stats current_stats;
        int delta;

        if(fetch_kernel_drop_stats(&current_stats) < 0) {
                printf("\n\nfetch kernel drop stats error!\n\n");
                exit(1);
        }       

        delta = current_stats.udp_errors - last_stats->udp_errors;
        printf("current_stats.udp_errors = %ld, last_stats->udp_errors = %ld\n", \
                current_stats.udp_errors, last_stats->udp_errors);
        *last_stats = current_stats;
        
        return delta > 0; 
} 
