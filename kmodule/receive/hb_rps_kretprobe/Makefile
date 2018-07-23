EXTRA_CFLAGS := -I./

obj-m += hb_rps_kretprobe.o 

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
