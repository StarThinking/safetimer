#!/bin/bash

# calculate num of 99% and 95% latencies
init_lat_arrays() {
    count=$1
    lat99_num=$[$count * 1 / 100]
    lat95_num=$[$count * 5 / 100]
    for i in $(seq $lat99_num)
    do
        lat99s[$i]=0
    done
    for i in $(seq $lat95_num)
    do
        lat95s[$i]=0
    done
}

update_array() {
    name=$1[@]
    array=("${!name}")
    x=$2
    for index in ${!array[@]}
    do
        if [ $x -gt ${array[$index]} ]
        then
            array[$index]=$x
        fi
    done
}

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
    lat99_num=0
    lat95_num=0

    for lat in $lats
    do
        count=$[$count + 1]
    done
    #init_lat_arrays $count
    #update_array lat95s

    for lat in $lats
    do
        lat=${lat#-}
        sum=$[$sum + $lat]
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

