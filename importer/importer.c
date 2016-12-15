#include <stdio.h>

#include "ipfs/importer/importer.h"
#include "ipfs/merkledag/merkledag.h"

#define MAX_DATA_SIZE 262144 // 1024 * 256;

/***
 * Imports OS files into the datastore
 */

/**
 * read the next chunk of bytes, create a node, and add a link to the node in the passed-in node
 * @param file the file handle
 * @param node the node to add to
 * @returns number of bytes read
 */
size_t ipfs_import_chunk(FILE* file, struct Node* node, struct FSRepo* fs_repo) {
	unsigned char buffer[MAX_DATA_SIZE];
	size_t bytes_read = fread(buffer, 1, MAX_DATA_SIZE, file);

	if (node->data_size == 0) {
		ipfs_node_set_data(node, buffer, bytes_read);
	} else {
		// create a new node, and link to the parent
		struct Node* new_node = NULL;
		ipfs_node_new_from_data(buffer, bytes_read, &new_node);
		// persist
		ipfs_merkledag_add(new_node, fs_repo);
		// put link in parent node
		struct NodeLink* new_link = NULL;
		ipfs_node_link_new("", new_node->cached->hash, new_node->cached->hash_length, &new_link);
		ipfs_node_add_link(node, new_link);
		ipfs_node_free(new_node);
	}
	if (bytes_read != MAX_DATA_SIZE) {
		// persist the main node
		ipfs_merkledag_add(node, fs_repo);
	}
	return bytes_read;
}

/**
 * Creates a node based on an incoming file
 * @param file_name the file to import
 * @param node the root node (could have links to others)
 * @returns true(1) on success
 */
int ipfs_import_file(const char* fileName, struct Node** node, struct FSRepo* fs_repo) {
	int retVal = 1;
	int bytes_read = MAX_DATA_SIZE;

	FILE* file = fopen(fileName, "rb");
	retVal = ipfs_node_new(node);
	if (retVal == 0)
		return 0;

	// add all nodes
	while ( bytes_read == MAX_DATA_SIZE) {
		bytes_read = ipfs_import_chunk(file, *node, fs_repo);
	}
	fclose(file);

	return 1;
}
