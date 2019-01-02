CC = gcc
CFLAGS = -O0 -I../include -I../c-libp2p/include -I../c-libp2p/c-multiaddr/include -I../c-libp2p/c-protobuf -Wall

ifdef DEBUG
CFLAGS += -g3
endif

LFLAGS =
DEPS = 
OBJS = cli.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

ipfs: $(OBJS)
	$(CC) -o $@ $^ $(LFLAGS)
	
all: $(OBJS)
	cd ipfs; make all;

clean:
	rm -f *.o
	cd ipfs; make clean;
