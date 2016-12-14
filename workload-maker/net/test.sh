path=/root/hb-latency/workload-maker/net

server="10.0.0.12"
clients="10.0.0.11"

datasize=1000
duration=30000

ssh $server "cd $path; killall ./server; ./server $datasize" &

sleep 5

for ((i=0; i<10; i++))
do
  ssh $clients "cd $path; ./client $server $datasize $duration" & 
done
