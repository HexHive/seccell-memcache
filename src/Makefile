CFLAGS += -g -fcf-protection=none -O2 -static      
LDFLAGS += -static
OBJS := main.o cache.o jenkins_hash.o slabs.o

main: ${OBJS}

.PHONY: clean
clean:
	rm -f ${OBJS}