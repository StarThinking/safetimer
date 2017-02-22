#!/bin/bash

path=/root/hb-latency/workload-maker

disk_workload_cleanup() {
    echo "disk_workload_cleanup"

    echo "killall hackbench"
    killall hackbench

#    echo "killall bonnie++"
 #   killall bonnie++
    rm -rf ~/tmp

    echo "killall wget and apache2" 
    killall wget
    killall apache2
    sleep 1
}

disk_workload_init() {
    echo "disk_workload_init"
    local duration=$1
    echo "launch wget+hackbench+bonnie for "$duration"s on ALL CPUs!"

    service apache2 restart
    mkdir ~/tmp
    sleep 1

    for((i=1; i<=8; i++))
    do
        taskset 0x$i timeout "$duration"s $path/disk/run_wget.sh &
        taskset 0x$i timeout "$duration"s $path/disk/run_hackbench.sh &
 #       taskset 0x$i timeout "$duration"s $path/disk/run_bonnie++.sh &
    done
}

disk_workload_cleanup
disk_workload_init 30
disk_workload_cleanup
