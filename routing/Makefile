CC = gcc
CFLAGS = -O0 -I../include -I../c-libp2p/include -I../c-libp2p/c-multiaddr/include -I../c-libp2p/c-protobuf -Wall -std=gnu99

ifdef DEBUG
CFLAGS += -g3
endif

LFLAGS = 
DEPS = 
OBJS = offline.o online.o k_routing.o supernode.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJS)

clean:
	rm -f $(OBJS)
