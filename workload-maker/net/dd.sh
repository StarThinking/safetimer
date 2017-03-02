suffix=$1
while true
do
    touch /tmp/tmp$suffix
    dd if=/dev/urandom of=/tmp/tmp$suffix count=1 bs=4K
    rm /tmp/tmp$suffix
done
