file=$1

lats=`cat $file | grep latency | awk 'BEGIN{FS=",| "}{print $13}'`

sum=0
count=0
for lat in $lats
do
    lat=${lat#-}
    sum=$[$sum + $lat]
    count=$[$count + 1]
done

avg=$[$sum / $count]
echo "avg latency = $avg ns"
