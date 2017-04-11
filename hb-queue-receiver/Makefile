all: hb_queue

hb_queue:
	gcc hb_queue.c -Wall -lrt -L. -lhashtable -D__WITH_MURMUR -lpthread -lnfnetlink -lnetfilter_queue -o hb_queue

clean:
	rm hb_queue
