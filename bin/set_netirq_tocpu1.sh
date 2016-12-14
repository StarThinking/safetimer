for irq in 48 49 50 51 52
do
    echo 02 > /proc/irq/$irq/smp_affinity
    echo 1 > /proc/irq/$irq/smp_affinity_list
done
