path=/root/hb-latency/workload-maker/net

server="node220-2"
clients="node220-1"

datasize=1000
duration=30000

ssh $server "cd $path; killall ./server; ./server $datasize" &

sleep 5

for ((i=0; i<20; i++))
do
  ssh $clients "cd $path; ./client $server $datasize $duration" & 
done
