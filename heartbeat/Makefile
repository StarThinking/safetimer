CC = gcc

CFLAGS = -Wall -ggdb -D__WITH_MURMUR

INCLUDE = -I./include

all: bin/receiver bin/sender

#bin/hb_queue: src/hb_queue.o src/drop.o src/utility.o
#	@mkdir -p bin
#	$(CC) $^ -L./lib -lhashtable -lrt -lpthread -lnfnetlink -lnetfilter_queue lib/liblist.a -o $@

bin/receiver: src/receiver.o src/barrier.o src/hb_server.o src/queue.o src/helper.o
	@mkdir -p bin
	$(CC) $^ -lpthread -o $@

bin/sender: src/sender.o src/helper.o
	@mkdir -p bin
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDE) -c -o $@

clean:
	rm -rf build bin *.o src/*.o

.PHONY: clean
