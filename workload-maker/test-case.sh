#!/bin/bash

dir=''
run=0
time=0
cpu=false
disk=false
net=false
server_type=regular
help="Usage: ./test-case [OPTIONS] -d [dir] -r [run_num] -t [second] Options: -cpu, -disk, -net, --slow, --none"

if [ $# == 0 ]
then
    echo "$help"
    exit 
fi

while [ -n "$1" ]
do
    case "$1" in
        -d) dir=$2
            shift;;
        
        -r) run=$2
            shift;;

        -t) time=$2
            shift;;
            
        -cpu) cpu=true
            ;;
       
        -disk) disk=true
            ;;
        
        -net) net=true
            ;;
        
        --slow) server_type=slow
            shift;;
        
        --none) server_type=none
            shift;;

        --help) echo $help
            exit;;
        
        *) echo "Wrong Arguments! $help"
            exit;;
    esac
    shift
done

echo "Test Case: dir=$dir, run=$run, time=$time, cpu=$cpu, disk=$disk, net=$net, server_type=$server_type" | tee ./$dir/readme

# cpu assignment
echo "assign all net workloads to cpu1"
/root/hb-latency/bin/set_netirq_tocpu1.sh

for((r=1; r<=$run; r++))
do  
    echo "Test Case Run $r: dir=$dir, run=$run, time=$time, cpu=$cpu, disk=$disk, net=$net, server_type=$server_type"
    ./test.sh $dir $r $time $cpu $disk $net $server_type
done
