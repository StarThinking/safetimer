obj-m   := my_debugfs.o
KDIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)
 
all:
	                $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
