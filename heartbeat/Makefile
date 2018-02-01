CC = gcc

CFLAGS = -fPIC -shared -Wall -ggdb -D__WITH_MURMUR

INCLUDE = -I./include -I/usr/lib/jvm/java-7-openjdk-amd64/include

all: receiver sender

receiver: src/receiver.o src/barrier.o src/hb_server.o src/state_server.o src/queue.o src/helper.o src/drop.o
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/libreceiver.so $^ -lc
	#$(CC) $^ -L./lib -lnfnetlink -lnetfilter_queue -lpthread -lrt ./lib/liblist.a -lhashtable -o $@

sender: src/sender.o src/request_server.o src/helper.o
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/libsender.so $^ -lc

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDE) -c -o $@

clean:
	rm -rf build/*.so *.o src/*.o

.PHONY: clean
