CC = gcc

CFLAGS = -fPIC -Wall -ggdb -D__WITH_MURMUR

INCLUDE = -I./include

all: build/libreceiver.so

build/libreceiver.so: src/receiver.o src/barrier.o src/hb_server.o src/queue.o src/helper.o
	@mkdir -p build
	$(CC) -shared -fPIC -o $@ $^ -lc
	#$(CC) $^ -L./lib -lnfnetlink -lnetfilter_queue -lpthread -lrt -o $@

build/libsender.so: src/sender.o src/helper.o
	@mkdir -p build
	$(CC) -shared -fPIC -o $@ $^ -lc

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDE) -c -o $@

clean:
	rm -rf build/*.so *.o src/*.o

.PHONY: clean
