CC = gcc

CFLAGS = -fPIC -Wall -ggdb -D__WITH_MURMUR

INCLUDE = -I./include

all: receiver sender

receiver: src/receiver.o src/barrier.o src/hb_server.o src/queue.o src/helper.o
	@mkdir -p build
	$(CC) -shared -fPIC -o build/libreceiver.so $^ -lc
	#$(CC) $^ -L./lib -lnfnetlink -lnetfilter_queue -lpthread -lrt -o $@

sender: src/sender.o src/helper.o
	@mkdir -p build
	$(CC) -shared -fPIC -o build/libsender.so $^ -lc

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDE) -c -o $@

clean:
	rm -rf build/*.so *.o src/*.o

.PHONY: clean
