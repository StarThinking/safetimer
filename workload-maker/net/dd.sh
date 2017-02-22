suffix=$1
while true
do
    dd if=/dev/urandom of=tmp$suffix count=1 bs=4K
    rm tmp$suffix
done
