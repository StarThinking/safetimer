#!/bin/bash
file_num=$#
for((i=1; i<=$file_num; i++))
do
    files[$i]=$1
#    echo ${files[$i]}
    shift
done

avg_avg=0
max_max=0

for((i=1; i<=$file_num; i++))
do
    lats=`tail -n +3 ${files[$i]} | grep latency | awk 'BEGIN{FS=",| "}{print $13}'`
    sum=0
    count=0
    max=0
    for lat in $lats
    do
        lat=${lat#-}
        sum=$[$sum + $lat]
        count=$[$count + 1]
        if [ $lat -gt $max ] 
        then
            max=$lat
        fi
    done
    echo "avg = $[$sum / $count], max = $max"
    avg_avg=$[$avg_avg + $[$sum / $count]]
    if [ $max -gt $max_max ]
    then
        max_max=$max
    fi
done

echo "avg latency = $[$avg_avg / $file_num] ns, max latency = $max_max ns"
