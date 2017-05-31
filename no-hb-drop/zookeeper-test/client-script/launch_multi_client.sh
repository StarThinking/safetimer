for zc in 100 200 400 800 1600 3200 6400 12800 25600 51200
do
    count=0
    while [ $count -lt 20 ]
    do
    
    for n in 3 4
    do
        ssh node220-$n "cd $(pwd); PYTHONPATH=lib.linux-x86_64-2.6 LD_LIBRARY_PATH=lib.linux-x86_64-2.6 ./zk-latencies.py --root_znode=/client$n --cluster "10.0.0.11:2181" --znode_size=100 --znode_count=$zc --timeout=5000 watch_multiple=5" &
        
        client_pids[$n]=$!
    done

    for n in 3 4
    do
        wait ${client_pids[$n]}
        echo "node220-$n finishes the $count test with zc $zc"
    done

    count=$[$count + 1]
    done
done
