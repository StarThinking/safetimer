RESULTPATH=$1
client_num=$2
run=$3
STRESS=$4
path=`pwd`
server="10.0.0.12"
client="10.0.0.11"
datasize=8
duration=30000

echo "start $run run of test with client_num $client_num"
 
cd $path; killall ./server; kill -9 $(lsof -i:5000 -t); taskset 0x2 ./server $datasize &

if [ $STRESS == 'true' ] 
then
    echo "add cpu and mem workload"
    for i in `seq 1 10`
    do
        taskset 0x2 stress --cpu 2 --vm 2 --vm-bytes 128M --timeout 30s &
    done
else
    echo "donnot add cpu and mem workload"
fi
sleep 3

ssh $client "cd $path; killall client; ./launch_client.sh $client_num $server $datasize $duration" &
sleep 3

dmesg -c > /dev/null
ssh $client "cd $path; rm -rf cpu.tmp; touch cpu.tmp; sar -u 1 20 > cpu.tmp" &
ssh $client "cd $path; rm -rf net.tmp; touch net.tmp; sar -n DEV 1 20 > net.tmp" &
sar -P 1 1 20 > ./$RESULTPATH/$client_num/server.cpu.$run &
sar -n DEV 1 20 > ./$RESULTPATH/$client_num/server.net.$run &
sleep 20
sleep 2
dmesg -c > ./$RESULTPATH/$client_num/lat.$run

sleep 10
scp $client:$path/cpu.tmp ./$RESULTPATH/$client_num/client.cpu.$run 
scp $client:$path/net.tmp ./$RESULTPATH/$client_num/client.net.$run 
