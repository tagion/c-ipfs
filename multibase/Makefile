CC = gcc
CFLAGS = -O0 -I../include -I../c-libp2p/include -I../c-libp2p/c-multihash/include -Wall -std=gnu99

ifdef DEBUG
CFLAGS += -g3
endif

LFLAGS = 
DEPS = 
OBJS = multibase.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJS)

clean:
	rm -f *.o
