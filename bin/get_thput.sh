file=$1

ths=`cat $file | grep thoughput | awk 'BEGIN{FS="=| "}{print $2}'`

sum=0
count=0
for th in $ths
do
    sum=$[$sum + $th]
    count=$[$count + 1]
done

echo "total thoughput = $sum kb/s"
