#!/bin/bash

# assign all net workloads to cpu1
echo "assign all net workloads to cpu1"
/root/hb-latency/bin/set_netirq_tocpu1.sh

cli_num[1]=40
SIZE=${#cli_num[@]}
echo "SIZE is $SIZE" 
