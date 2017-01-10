#!/bin/bash

server="10.0.0.12"
client="10.0.0.11"
datasize=8
client_num=40
workpath=/root/hb-latency/workload-maker/net

net_workload_cleanup() {
    echo "net_workload_cleanup"
    echo "kill server"
    killall server; kill -9 $(lsof -i:5000 -t);
    echo "kill clients"
    ssh $client "killall client"
}

net_workload_init() {
    echo "net_workload_init"
    echo "launch server"
    cd $workpath; taskset 0x2 ./server $datasize &
    sleep 1
    echo "launch clients"
    ssh $client "cd $workpath; ./launch_client.sh $client_num $server $datasize" &
}

