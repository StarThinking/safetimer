#!/bin/bash

# assign all net workloads to cpu1
echo "assign all net workloads to cpu1"
/root/hb-latency/bin/set_netirq_tocpu1.sh

cli_num[1]=40
cli_num[2]=80
cli_num[3]=120
cli_num[4]=160
cli_num[5]=200
SIZE=${#cli_num[@]}
echo "SIZE is $SIZE" 
