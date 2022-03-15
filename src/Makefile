INC=-Imbedtls/include
CFLAGS += -g -fcf-protection=none -O2 -static ${INC}
LDFLAGS += -static -Lmbedtls/lib 
LDLIBS += -lmbedcrypto
OBJS := main.o cache.o jenkins_hash.o slabs.o cache_wrapper.o

main: ${OBJS}

.PHONY: clean
clean:
	rm -f ${OBJS}