CC = gcc

CFLAGS = -fPIC -shared -Wall -ggdb -D__WITH_MURMUR

INCLUDE = -I./include

all: receiver sender

receiver: src/receiver.o src/barrier.o src/hb_server.o src/queue.o src/helper.o
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/libreceiver.so $^ -lc
	#$(CC) $^ -L./lib -lnfnetlink -lnetfilter_queue -lpthread -lrt ./lib/liblist.a -lhashtable -o $@

sender: src/sender.o src/helper.o
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/libsender.so $^ -lc

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDE) -c -o $@

clean:
	rm -rf build/*.so *.o src/*.o

.PHONY: clean
