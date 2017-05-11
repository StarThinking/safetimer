SRCDIR = ./src
INCLUDE = -I./include

all: hb_queue self_sender hb_receiver udp_sender

utility:
		gcc -c $(SRCDIR)/utility.c $(INCLUDE)

hb_queue: utility
		gcc $(SRCDIR)/hb_queue.c utility.o -Wall $(INCLUDE) -L./lib -lhashtable -lrt -D__WITH_MURMUR -lpthread -lnfnetlink -lnetfilter_queue -o hb_queue

self_sender:
		gcc $(SRCDIR)/self_sender.c $(INCLUDE) -lpthread -o self_sender

hb_receiver:
		gcc $(SRCDIR)/hb_receiver.c $(INCLUDE) -lpthread -o hb_receiver

udp_sender:
		gcc $(SRCDIR)/udp_sender.c $(INCLUDE) -o udp_sender

clean:
		rm hb_queue self_sender hb_receiver udp_sender *.o
