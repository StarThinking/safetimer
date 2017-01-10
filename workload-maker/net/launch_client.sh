num=$1
server=$2
datasize=$3

#killall ./client

for((i=0; i<$num; i++))
do
    echo start client $i
    ./client $server $datasize &
    pids[$i]=$!
done

for((j=0; j<$num; j++))
do
    wait ${pids[$j]}
    #echo "thput of client $j is ${thputs[$j]}"
done

echo "all clients finish"

