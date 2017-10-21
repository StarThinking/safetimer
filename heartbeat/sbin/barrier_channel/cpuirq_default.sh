BASE_IRQ=49

echo ff > /proc/irq/$BASE_IRQ/smp_affinity
echo ff > /proc/irq/$(( $BASE_IRQ + 1 ))/smp_affinity
echo ff > /proc/irq/$(( $BASE_IRQ + 2 ))/smp_affinity
echo ff > /proc/irq/$(( $BASE_IRQ + 3 ))/smp_affinity

echo 0-7 > /proc/irq/$BASE_IRQ/smp_affinity_list
echo 0-7 > /proc/irq/$(( $BASE_IRQ + 1 ))/smp_affinity_list
echo 0-7 > /proc/irq/$(( $BASE_IRQ + 2 ))/smp_affinity_list
echo 0-7 > /proc/irq/$(( $BASE_IRQ + 3 ))/smp_affinity_list

cat /proc/irq/$BASE_IRQ/smp_affinity*
cat /proc/irq/$(( $BASE_IRQ + 1 ))/smp_affinity*
cat /proc/irq/$(( $BASE_IRQ + 2 ))/smp_affinity*
cat /proc/irq/$(( $BASE_IRQ + 3 ))/smp_affinity*
