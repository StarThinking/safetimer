all: hb_queue self_sender

hb_queue:
	gcc hb_queue.c -Wall -lrt -L. -lhashtable -D__WITH_MURMUR -lpthread -lnfnetlink -lnetfilter_queue -o hb_queue

self_sender:
	gcc self_sender.c -lpthread -o self_sender

clean:
	rm hb_queue self_sender
