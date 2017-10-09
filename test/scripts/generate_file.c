#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	if (argc != 3) {
		fprintf(stderr, "Syntax: %s <filename> <size>\n", argv[0]);
		exit(1);
	}

	char* filename = argv[1];
	long file_size = atoi(argv[2]);

	FILE* fd = fopen(filename, "w");
	if (!fd) {
		fprintf(stderr, "Unable to open the file %s for writing.\n", filename);
		exit(1);
	}

	for(size_t i = 0; i < file_size; i++) {
		char byte = i % 255;
		fwrite(&byte, 1, 1, fd);
	}

	fclose(fd);
	return 0;
}
