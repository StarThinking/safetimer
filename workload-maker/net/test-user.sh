client_num=$1
run=$2
path=/root/hb-latency/workload-maker/net
server="10.0.0.12"
client="10.0.0.11"
datasize=10
duration=30000

echo "start $run run of test with client_num $client_num"
 
ssh $server "cd $path; killall ./server; taskset 0x2 ./server $datasize" &
sleep 3

ssh $client "cd $path; ./launch_client.sh $client_num $server $datasize $duration" &
sleep 3

echo > /tmp/sock.log
ssh $client "cd $path; rm -rf cpu.tmp; touch cpu.tmp; sar -u 1 20 > cpu.tmp" &
ssh $client "cd $path; rm -rf net.tmp; touch net.tmp; sar -n DEV 1 20 > net.tmp" &
sar -P 1 1 20 > ./result/$client_num/server.cpu.$run &
sar -n DEV 1 20 > ./result/$client_num/server.net.$run &
sleep 20
sleep 2
cp /tmp/sock.log ./result/$client_num/lat.$run
echo > /tmp/sock.log
scp $client:$path/cpu.tmp ./result/$client_num/client.cpu.$run 
scp $client:$path/net.tmp ./result/$client_num/client.net.$run 

sleep 10
