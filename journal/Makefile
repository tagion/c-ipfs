CC = gcc
CFLAGS = -O0 -I../include -I../c-libp2p/include -I../c-libp2p/c-multiaddr/include -I../c-libp2p/c-multihash/include -I../c-libp2p/c-protobuf -I../lmdb/libraries/liblmdb -Wall -std=gnu99

ifdef DEBUG
CFLAGS += -g3
endif

LFLAGS = 
DEPS = 
OBJS = journal.o journal_entry.o journal_message.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJS)

clean:
	rm -f *.o
