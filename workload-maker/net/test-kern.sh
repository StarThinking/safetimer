#!/bin/bash

RESULTPATH=$1
client_num=$2
run=$3
STRESS=$4
datasize=$5
path=`pwd`
server="10.0.0.12"
client="10.0.0.11"
stress_num=100
duration_s=600 #10min
duration_ms=$[duration_s * 1000]
sar_interval=10
sar_count=$[duration_s / $sar_interval]

echo "start $run run of test with client_num $client_num"
 
# cleanup
ssh $client "killall client"
killall ./server; kill -9 $(lsof -i:5000 -t); killall stress
sleep 1
dmesg -c > /dev/null

# launch server
cd $path; taskset 0x2 ./server $datasize &
sleep 1
# launch cpu/mem workload
if [ $STRESS == 'true' ] 
then
    echo "add cpu and mem workload"
    for i in `seq 1 $stress_num`
    do
        taskset 0x2 stress --cpu 2 --vm 2 --vm-bytes 16M --timeout "$duration_s"s &
    done
fi
sleep 1

# launch client workload and run for duration of time
ssh $client "cd $path; ./launch_client.sh $client_num $server $datasize $duration_ms" &
# launch sar monitor
ssh $client "cd $path; rm -rf cpu.tmp; touch cpu.tmp; sar -u $sar_interval $sar_count > cpu.tmp" &
ssh $client "cd $path; rm -rf net.tmp; touch net.tmp; sar -n DEV $sar_interval $sar_count > net.tmp" &
sar -P 1 $sar_interval $sar_count > ./$RESULTPATH/$client_num/server.cpu.$run &
sar -n DEV $sar_interval $sar_count > ./$RESULTPATH/$client_num/server.net.$run &
dmesg -c > /dev/null
buffer_time=10
sleep $[$duration_s - $buffer_time]
dmesg -c > ./$RESULTPATH/$client_num/lat.$run
sleep $buffer_time

# cleanup
ssh $client "killall client"
killall ./server; kill -9 $(lsof -i:5000 -t); killall stress; sleep 1
dmesg -c > /dev/null

# calm down
sleep 60
scp $client:$path/cpu.tmp ./$RESULTPATH/$client_num/client.cpu.$run 
scp $client:$path/net.tmp ./$RESULTPATH/$client_num/client.net.$run 
