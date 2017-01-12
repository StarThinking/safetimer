#!/bin/bash

path=/root/hb-latency/workload-maker

source $path/cpu_workload.sh
source $path/disk_workload.sh
source $path/net_workload.sh

dir=$1
run=$2
time=$3
cpu=$4
disk=$5
net=$6
server_type=$7
sar_int=10
sar_count=$[$[$time / $sar_int] - 1]

echo "workload cleanup"
cpu_workload_cleanup
disk_workload_cleanup
net_workload_cleanup

dmesg -c > /dev/null

# launch workload, remember that all the workloads on server side should be placed the 2nd cpu
if [ $cpu == 'true' ]
then
    cpu_workload_init
fi

if [ $disk == 'true' ]
then
    disk_workload_init $time
fi

if [ $net == 'true' ]
then
    net_workload_init $server_type $time
    client_sar_init $sar_int $sar_count $dir $run
fi

echo "launch server sar monitor"
echo "dir = $dir"
sar -P 1 $sar_int $sar_count > $dir/server.cpu.$run &
sar -n DEV $sar_int $sar_count > $dir/server.net.$run &

echo "start recording"
dmesg -c > /dev/null
BUF_S=10
sleep $[$time - $BUF_S]
dmesg -c > $dir/lat.$run
sleep $BUF_S
sleep $BUF_S

echo "workload cleanup"
cpu_workload_cleanup
disk_workload_cleanup
net_workload_cleanup
dmesg -c > /dev/null

echo "system calm down"
sleep 30

if [ $net == 'true' ]
then
    # scp monitor info from client
    client_sar_collect
fi
