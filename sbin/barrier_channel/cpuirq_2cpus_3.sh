BASE_IRQ=33

echo 01 > /proc/irq/$BASE_IRQ/smp_affinity
echo 01 > /proc/irq/$(( $BASE_IRQ + 1 ))/smp_affinity
echo 01 > /proc/irq/$(( $BASE_IRQ + 2 ))/smp_affinity
echo 02 > /proc/irq/$(( $BASE_IRQ + 3 ))/smp_affinity

echo 0 > /proc/irq/$BASE_IRQ/smp_affinity_list
echo 0 > /proc/irq/$(( $BASE_IRQ + 1 ))/smp_affinity_list
echo 0 > /proc/irq/$(( $BASE_IRQ + 2 ))/smp_affinity_list
echo 1 > /proc/irq/$(( $BASE_IRQ + 3 ))/smp_affinity_list

cat /proc/irq/$BASE_IRQ/smp_affinity*
cat /proc/irq/$(( $BASE_IRQ + 1 ))/smp_affinity*
cat /proc/irq/$(( $BASE_IRQ + 2 ))/smp_affinity*
cat /proc/irq/$(( $BASE_IRQ + 3 ))/smp_affinity*

echo 3 > /sys/kernel/debug/10.0.0.11/queue_index
