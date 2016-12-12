CC = gcc
CFLAGS = -O0 -I../include -I../../c-libp2p/include -I../../c-protobuf -Wall
LFLAGS = 
DEPS = builder.h ipfs_node.h
OBJS = builder.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJS)

clean:
	rm -f *.o
