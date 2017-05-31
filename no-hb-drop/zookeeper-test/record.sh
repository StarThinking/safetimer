file=$1

netstat -s > "$file"_test_netstat.txt

ifconfig > "$file"_test_ifconfig.txt

ethtool -S em1 > "$file"_test_ethtool.txt



