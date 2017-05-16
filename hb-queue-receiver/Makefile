CC = gcc

CFLAGS = -Wall -ggdb -D__WITH_MURMUR

INCLUDE = -I./include

SRCS = src/utility.c

OBJS = $(SRCS:.c=.o)

all: bin/hb_queue bin/self_sender bin/hb_receiver bin/udp_sender

bin/hb_queue: src/hb_queue.o $(OBJS)
	@mkdir -p bin
	$(CC) $^ -L./lib -lhashtable -lrt -lpthread -lnfnetlink -lnetfilter_queue lib/liblist.a -o $@

bin/self_sender: src/self_sender.o
	@mkdir -p bin
	gcc $^ -lpthread -o $@

bin/hb_receiver: src/hb_receiver.o
	@mkdir -p bin
	$(CC) $^ -lpthread -o $@

bin/udp_sender: src/udp_sender.o
	@mkdir -p bin
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDE) -c -o $@

clean:
	rm -rf build bin *.o src/*.o

.PHONY: clean
