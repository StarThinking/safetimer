cmd_/root/hb-latency/no-hb-drop/nic/nic_drop.ko := ld -r -m elf_x86_64 -T /usr/src/linux-3.14.33-regular/scripts/module-common.lds --build-id  -o /root/hb-latency/no-hb-drop/nic/nic_drop.ko /root/hb-latency/no-hb-drop/nic/nic_drop.o /root/hb-latency/no-hb-drop/nic/nic_drop.mod.o