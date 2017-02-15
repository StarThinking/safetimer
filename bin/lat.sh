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
valid_file_num=$file_num
#lat_position=14

for((i=1; i<=$file_num; i++))
do
    lats=`tail -n +3 ${files[$i]} | grep latency | awk 'BEGIN{FS=",| "}{print ($13 >= 0) ? $14 : 0-$14}' | sort -rn`
    sum=0
    count=0
    max=0
    lat999_num=0
    lat99_num=0
    lat95_num=0
    lat999=0
    lat99=0;
    lat95=0;

    for lat in $lats
    do
#        echo $lat
        count=$[$count + 1]
    done
    lat999_num=$[$count * 1 / 1000]
    lat99_num=$[$count * 1 / 100]
    lat95_num=$[$count * 5 / 100]

    count=0
    for lat in $lats
    do
#        echo $lat
        count=$[$count + 1]
        sum=$[$sum + $lat]
        if [ $lat -gt $max ]
        then
            max=$lat
        fi
        if [ $count -eq $lat999_num ]
        then
            lat999=$lat
        fi
        if [ $count -eq $lat99_num ]
        then
            lat99=$lat
        fi
        if [ $count -eq $lat95_num ]
        then
            lat95=$lat
        fi
    done
    
    if [ $count -eq 0 ]
    then
        valid_file_num=$[$valid_file_num - 1]
        continue
    fi

    echo "avg = $[$sum / $count], max = $max, 99.9% = $lat999, 99% = $lat99, 95% = $lat95"
    avg_avg=$[$avg_avg + $[$sum / $count]]
    if [ $max -gt $max_max ]
    then
        max_max=$max
    fi
done

echo "avg latency = $[$avg_avg / $valid_file_num] ns, max latency = $max_max ns"
