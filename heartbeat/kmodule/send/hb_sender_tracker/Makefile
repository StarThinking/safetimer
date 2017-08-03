EXTRA_CFLAGS := -I./

obj-m += hb_sender_tracker.o 
hb_sender_tracker-objs := hb_sender_debugfs.o hb_sender_kretprobe.o hb_sender_netfilter.o hb_sender_main.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
