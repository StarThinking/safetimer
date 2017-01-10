#!/bin/bash

DIR=''
RUN=0
TIME=0
CPU=false
DISK=false
NET=false
HELP="Usage: ./test-case [OPTIONS] -d [DIR] -r [RUN_NUM] -t [SECOND] Options: -cpu, -disk, -net"

if [ $# == 0 ]
then
    echo "$HELP"
    exit 
fi

while [ -n "$1" ]
do
    case "$1" in
        -d) DIR=$2
            shift;;
        
        -r) RUN=$2
            shift;;

        -t) TIME=$2
            shift;;
            
        -cpu) CPU=true
            ;;
       
        -disk) DISK=true
            ;;
        
        -net) NET=true
            ;;

        --help) echo $HELP
            exit;;
        
        *) echo "Wrong Arguments! $HELP"
            exit;;
    esac
    shift
done

echo "Test Case: DIR=$DIR, RUN=$RUN, TIME=$TIME, CPU=$CPU, DISK=$DISK, NET=$NET" | tee ./$DIR/readme

# cpu assignment
echo "assign all net workloads to cpu1"
/root/hb-latency/bin/set_netirq_tocpu1.sh

for((r=1; r<=$RUN; r++))
do  
    echo "Test Case Run $r: DIR=$DIR, RUN=$RUN, TIME=$TIME, CPU=$CPU, DISK=$DISK, NET=$NET"
    ./test.sh $DIR $r $TIME $CPU $DISK $NET
done
