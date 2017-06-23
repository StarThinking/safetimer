EXTRA_CFLAGS := -I./

obj-m += hb_send_tracker.o 
hb_send_tracker-objs := hb_sender_debugfs.o hb_sender_main.o

#obj-m += hb_sender_debugfs.o 

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
