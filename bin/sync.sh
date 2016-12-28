host=10.0.0.11

echo "sync hb-latency to $host"

ssh $host "cd ~; rm -rf hb-latency"
scp -r ~/hb-latency $host:~
