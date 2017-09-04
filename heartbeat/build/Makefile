CC = gcc

CFLAGS = -Wall -ggdb -D__WITH_MURMUR

INCLUDE = -I../include

all: receiver_app sender_app

receiver_app: receiver_app.o
	$(CC) -o $@ $^ -L. -lreceiver -L../lib -lnfnetlink -lnetfilter_queue ../lib/liblist.a -lhashtable -lpthread -lrt

sender_app: sender_app.o
	$(CC) -o $@ $^ -L. -lsender -lpthread -lrt

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDE) -c -o $@

clean:
	rm *.o *_app

.PHONY: clean
