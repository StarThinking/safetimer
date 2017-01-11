#!/bin/bash

path=/root/hb-latency/workload-maker

disk_workload_cleanup() {
    echo "disk_workload_cleanup"

    echo "killall hackbench"
    killall hackbench

    echo "killall bonnie++"
    killall bonnie++
    rm -rf ~/tmp

    echo "killall wget and apache2" 
    killall wget
    killall apache2
    sleep 1
}

disk_workload_init() {
    echo "disk_workload_init"
    local duration=$1
    echo "launch wget hackbench bonnie for $duration s"

    service apache2 restart
    sleep 1
    taskset 0x2 timeout "$duration"s $path/disk/run_wget.sh &
    sleep 1

    taskset 0x2 timeout "$duration"s $path/disk/run_hackbench.sh &
    sleep 1

    mkdir ~/tmp
    taskset 0x2 timeout "$duration"s $path/disk/run_bonnie++.sh &
    sleep 1
}

