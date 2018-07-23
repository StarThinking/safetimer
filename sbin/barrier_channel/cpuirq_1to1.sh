BASE_IRQ=49

echo 01 > /proc/irq/$BASE_IRQ/smp_affinity
echo 02 > /proc/irq/$(( $BASE_IRQ + 1 ))/smp_affinity
echo 04 > /proc/irq/$(( $BASE_IRQ + 2 ))/smp_affinity
echo 08 > /proc/irq/$(( $BASE_IRQ + 3 ))/smp_affinity

echo 0 > /proc/irq/$BASE_IRQ/smp_affinity_list
echo 1 > /proc/irq/$(( $BASE_IRQ + 1 ))/smp_affinity_list
echo 2 > /proc/irq/$(( $BASE_IRQ + 2 ))/smp_affinity_list
echo 3 > /proc/irq/$(( $BASE_IRQ + 3 ))/smp_affinity_list

cat /proc/irq/$BASE_IRQ/smp_affinity*
cat /proc/irq/$(( $BASE_IRQ + 1 ))/smp_affinity*
cat /proc/irq/$(( $BASE_IRQ + 2 ))/smp_affinity*
cat /proc/irq/$(( $BASE_IRQ + 3 ))/smp_affinity*
