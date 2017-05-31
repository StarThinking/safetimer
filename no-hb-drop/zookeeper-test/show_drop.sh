echo "ethtool drop stats:"
ethtool -S em1 | egrep "rx_discards|rxbds_empty"

echo "" 

echo "rx_dropped stats:"
cat /proc/net/dev

echo "" 

echo "netstat drop stats:"
cat /proc/net/netstat | tail -n 2
