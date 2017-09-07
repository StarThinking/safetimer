CC = gcc

CFLAGS = -Wall -ggdb -D__WITH_MURMUR

INCLUDE = -I../include

all: receiver sender

receiver: receiver_app.o libreceiver.so
	$(CC) $(CFLAGS) -o receiver_app $^ -L../lib -lnfnetlink -lnetfilter_queue ../lib/liblist.a -lhashtable -lpthread -lrt

sender: sender_app.o libsender.so
	$(CC) $(CFLAGS) -o sender_app $^ -lpthread -lrt

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDE) -c -o $@

clean:
	rm *.o *_app

.PHONY: clean
