CFLAGS = -g
LDLIBS=-lrt
SRCS = tlbcache.c main.c
HEADERS = tlbcache.h config.h util.h
OBJS = ${SRCS:.c=.o}

.PHONY: all clean
all: main

main: ${OBJS} ${HEADERS}

clean:
	rm -f *.o
	rm -o main

