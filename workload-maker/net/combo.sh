net_num=$1
disk_num=$2
server=$3
datasize=$4
duration=$5

#killall ./client

for((i=0; i<$net_num; i++))
do
    echo start net client $i
    timeout "$duration"s ./client $server $datasize &
    net_pids[$i]=$!
done

for((i=0; i<$disk_num; i++))
do
    echo start dd thread $i
    ssh $server "cd /root/hb-latency/workload-maker/net; timeout "$duration"s ./dd.sh $disk_num" &
    disk_pids[$i]=$!
done

for((j=0; j<$net_num; j++))
do
    wait ${net_pids[$j]}
done

echo "all net workloads finish"

for((j=0; j<$disk_num; j++))
do
    wait ${disk_pids[$j]}
done

echo "all disk workloads finish"
