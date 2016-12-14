dir=/root/hb-latency/file-workload/
data_dir=/data/

rm -rf /data/
mkdir /data/

dir_num=300
file_num=10000

cd $dir
for((i=0; i<$dir_num; i++))
do
    echo "write $file_num files under directory $data_dir$i"
    
    #mkdir
    mkdir $data_dir$i

    #run process on cpu1
    taskset 0x2 ./writes $data_dir$i $file_num &
done
