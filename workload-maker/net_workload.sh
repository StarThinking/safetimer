#!/bin/bash

server="10.0.0.12"
client="10.0.0.11"
datasize=8
client_num=40
path=/root/hb-latency/workload-maker

net_workload_cleanup() {
    echo "net_workload_cleanup"
    echo "killall server"
    killall server; killall server_slow; kill -9 $(lsof -i:5000 -t);
    echo "killall client"
    local n=100
    while [ $n -gt 2 ]
    do
        ssh $client "killall client"
        sleep 1
        n=`ssh $client "ps aux | grep client | wc -l"`
    done
    sleep 1
}

net_workload_init() {
    local server_type=$1
    echo "net_workload_init"
    echo "launch server $server_type"
    if [ $server_type == 'regular' ]
    then
        taskset 0x2 $path/net/server $datasize &
    elif [ $server_type == 'slow' ]
    then
        taskset 0x2 $path/net/server_slow $datasize &
    elif [ $server_type == 'none' ]
    then
        echo "don't launch server!!"
    fi
    sleep 1
    echo "launch clients"
    ssh $client "cd $path/net; ./launch_client.sh $client_num $server $datasize" &
    sleep 2
}

client_sar_init() {
    local sar_int=$1
    local sar_count=$2
    local dir=$3
    local run=$4
    echo "client_sar_init"
    ssh $client "cd $path/net; rm -rf cpu.tmp; sar -u $sar_int $sar_count > cpu.tmp" &
    ssh $client "cd $path/net; rm -rf net.tmp; sar -n DEV $sar_int $sar_count >net.tmp" &
}

client_sar_collect() {
    ssh $client "cd $path/net; scp cpu.tmp $server:$dir/client.cpu.$run; client cpu sar finishes" 
    ssh $client "cd $path/net; scp net.tmp $server:$dir/client.net.$run; client net sar finishes" 
}
