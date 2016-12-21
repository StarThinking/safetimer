file_num=$#
for((i=1; i<=$file_num; i++))
do
    files[$i]=$1
#    echo ${files[$i]}
    shift
done

avg_avg=0
avg_max=0

for((i=1; i<=$file_num; i++))
do
    lats=`cat ${files[$i]} | grep latency | awk 'BEGIN{FS=",| "}{print $13}'`
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
    avg_max=$[$avg_max + $max]
done

echo "avg latency = $[$avg_avg / $file_num] ns, max latency = $[$avg_max / $file_num] ns"
