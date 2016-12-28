path=$1
echo "calculate the summary for $path"

cd $path
for entry in 0 20 40 60 80 100 120 140 160 180
do 
    echo "client number is $entry"
    
    echo "***Latency"
    ~/hb-latency/bin/lat.sh $entry/lat.*    

    echo "***Client Net"
    cat $entry/client.net.1 | egrep "Average" | egrep "em1|IFACE"
    
    echo "***Server Net"
    cat $entry/server.net.1 | egrep "Average" | egrep "em1|IFACE"
    
    echo "***Client CPU"
    cat $entry/client.cpu.1 | egrep "Average|idle"
   
    echo "***Server CPU"
    cat $entry/server.cpu.1 | egrep "Average|idle"
    echo ""
done
