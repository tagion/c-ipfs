CC = gcc

%.o: %.c
	$(CC) -c -o $@ $<

generate_file: generate_file.o
	$(CC) -o $@ $^ 

all: generate_file

clean:
	rm -f *.o
	rm -f generate_file
