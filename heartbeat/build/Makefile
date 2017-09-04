CC = gcc

CFLAGS = -Wall -ggdb -D__WITH_MURMUR

all: receiver_app

receiver_app: receiver_app.o
	$(CC) -o $@ $^ -L. -lreceiver -L../lib -lnfnetlink -lnetfilter_queue ../lib/liblist.a -lhashtable -lpthread -lrt

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

clean:
	rm *.o receiver_app

.PHONY: clean
