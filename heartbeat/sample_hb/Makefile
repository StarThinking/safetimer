CC = gcc

CFLAGS = -Wall -ggdb -D__WITH_MURMUR

INCLUDE = -I../include

all: receiver sender

receiver: receiver.o helper.o
	$(CC) $(CFLAGS) -o receiver $^ -lpthread -lrt ./liblist.a -L. -lhashtable

sender: sender.o helper.o
	$(CC) $(CFLAGS) -o sender $^ -lpthread -lrt

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDE) -c -o $@

clean:
	rm *.o receiver sender

.PHONY: clean
