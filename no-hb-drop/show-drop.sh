echo "packet drops in NIC"
ethtool -S em1 | egrep "rxbds_empty|rx_discards"

echo "packet drops in IP Stack"
netstat -s | grep error
