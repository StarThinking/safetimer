CC = gcc

CFLAGS = -Wall -ggdb -D__WITH_MURMUR

INCLUDE = -I./include

all: receiver sender

receiver: src/st_receiver.o src/barrier.o src/hb_server.o src/st_receiver_api.o src/queue.o src/helper.o src/drop.o
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/st_receiver $^ -L./lib -lnfnetlink -lnetfilter_queue ./lib/liblist.a -lhashtable -lpthread -lrt

sender: src/st_sender.o src/st_sender_api.o src/helper.o
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/st_sender $^ -lpthread -lrt

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDE) -c -o $@

clean:
	rm -rf build/* src/*.o 

.PHONY: clean
