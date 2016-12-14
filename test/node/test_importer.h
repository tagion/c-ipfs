#include <stdio.h>

#include "ipfs/importer/importer.h"
#include "ipfs/merkledag/merkledag.h"

/***
 * Helper to create a test file in the OS
 */
int create_file(const char* fileName, unsigned char* bytes, size_t num_bytes) {
	FILE* file = fopen(fileName, "wb");
	fwrite(bytes, num_bytes, 1, file);
	fclose(file);
	return 1;
}

int create_bytes(unsigned char* buffer, size_t num_bytes) {
	int counter = 0;

	for(int i = 0; i < num_bytes; i++) {
		buffer[i] = counter++;
		if (counter > 15)
			counter = 0;
	}
	return 1;
}

int test_import_small_file() {
	size_t bytes_size = 1000;
	unsigned char file_bytes[bytes_size];
	const char* fileName = "/tmp/test_import_small.tmp";

	// create the necessary file
	create_bytes(file_bytes, bytes_size);
	create_file(fileName, file_bytes, bytes_size);

	// get the repo
	drop_and_build_repository("/tmp/.ipfs");
	struct FSRepo* fs_repo;
	ipfs_repo_fsrepo_new("/tmp/.ipfs", NULL, &fs_repo);
	ipfs_repo_fsrepo_open(fs_repo);

	// write to ipfs
	struct Node* write_node;
	if (ipfs_import_file(fileName, &write_node, fs_repo) == 0) {
		ipfs_repo_fsrepo_free(fs_repo);
		return 0;
	}

	// make sure all went okay
	struct Node* read_node;
	if (ipfs_merkledag_get(write_node->cached, &read_node, fs_repo) == 0) {
		ipfs_repo_fsrepo_free(fs_repo);
		ipfs_node_free(write_node);
		return 0;
	}

	// compare data
	if (write_node->data_size != bytes_size || write_node->data_size != read_node->data_size) {
		printf("Data size of nodes are not equal or are incorrect. Should be %lu but are %lu\n", write_node->data_size, read_node->data_size);
		ipfs_repo_fsrepo_free(fs_repo);
		ipfs_node_free(write_node);
		ipfs_node_free(read_node);
		return 0;
	}

	for(int i = 0; i < bytes_size; i++) {
		if (write_node->data[i] != read_node->data[i]) {
			printf("Data within node is different at position %d\n", i);
			ipfs_repo_fsrepo_free(fs_repo);
			ipfs_node_free(write_node);
			ipfs_node_free(read_node);
			return 0;
		}
	}

	return 1;
}
