EXTRA_CFLAGS := -I./

obj-m += kprobe_latency.o 

all:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
