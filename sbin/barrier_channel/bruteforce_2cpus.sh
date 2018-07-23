BASE_IRQ=33
IRQ_NUM=4

# clear up
echo 0 > /sys/kernel/debug/10.0.0.11/clear

for rx_queue in $(seq 0 $(($IRQ_NUM - 1)))
do
	echo "let's find the connection for rx_queue $rx_queue"

	# map All rx queues to the First CPU
	for index in $(seq 0 $(($IRQ_NUM - 1)))
	do
		echo 01 > /proc/irq/$(( $BASE_IRQ + $index ))/smp_affinity # bitmap
		echo 0 > /proc/irq/$(( $BASE_IRQ + $index ))/smp_affinity_list # ids
		cat /proc/irq/$(( $BASE_IRQ + $index ))/smp_affinity*
	done

	# map only the "$rx_queue" to the Second CPU
	echo 02 > /proc/irq/$(( $BASE_IRQ + $rx_queue ))/smp_affinity
	echo 1 > /proc/irq/$(( $BASE_IRQ + $rx_queue ))/smp_affinity_list
        # communicate with the kernel module
	echo $rx_queue > /sys/kernel/debug/10.0.0.11/queue_index
	
	while true
	do
		sport=$(cat /sys/kernel/debug/10.0.0.11/$(( $BASE_IRQ + $rx_queue )))
		echo "sport for $rx_queue is $sport"
		if [ $sport -gt 0 ]
		then
			break
		else
			echo "sleep 1"
			sleep 1
		fi
	done
done

echo "We have established tcp connections for all the rx queues"
echo "Set the smp_affinity and smp_affinity_list to the default configurations"

# map All rx queues to the All CPU
for index in $(seq 0 $(($IRQ_NUM - 1)))
do
	echo ff > /proc/irq/$(( $BASE_IRQ + $index ))/smp_affinity # bitmap
	echo 0-7 > /proc/irq/$(( $BASE_IRQ + $index ))/smp_affinity_list # ids
	cat /proc/irq/$(( $BASE_IRQ + $index ))/smp_affinity*
done
