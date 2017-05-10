SRCDIR = ./src
INCLUDE = -I./include

all: queue 

queue:
	gcc queue.c -Wall -D__WITH_MURMUR -lnfnetlink -lnetfilter_queue -o queue

clean:
	rm queue
