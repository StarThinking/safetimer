#!/bin/bash

source cpu_workload.sh
source disk_workload.sh
source net_workload.sh

DIR=$1
RUN=$2
TIME=$3
CPU=$4
DISK=$5
NET=$6 
SAR_INT=10
SAR_COUNT=$[$[$TIME / $SAR_INT] - 1]

echo DIR = $DIR
echo "workload cleanup"
cpu_workload_cleanup
disk_workload_cleanup
net_workload_cleanup

dmesg -c > /dev/null

# launch workload, remember that all the workloads on server side should be placed the 2nd cpu
if [ $CPU == 'true' ]
then
    cpu_workload_init
fi

if [ $DISK == 'true' ]
then
    disk_workload_init
fi

if [ $NET == 'true' ]
then
    net_workload_init
fi

echo "launch server sar monitor"
sar -P 1 $SAR_INT $SAR_COUNT > ./$DIR/server.cpu.$RUN &
sar -n DEV $SAR_INT $SAR_COUNT > ./$DIR/server.net.$RUN &

echo "start recording"
dmesg -c > /dev/null
BUF_S=10
sleep $[$TIME - $BUF_S]
dmesg -c > ./$DIR/lat.$RUN
sleep $BUF_S

echo "system calm down"
sleep 60

echo "workload cleanup"
cpu_workload_cleanup
disk_workload_cleanup
net_workload_cleanup
dmesg -c > /dev/null

#ssh $client "cd $PATH; rm -rf cpu.tmp; touch cpu.tmp; sar -u $sar_interval $sar_count > cpu.tmp" &
# ssh $client "cd $PATH; rm -rf net.tmp; touch net.tmp; sar -n DEV $sar_interval $sar_count > net.tmp" &
