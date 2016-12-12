#include <stdio.h>

#include "ipfs/importer/importer.h"

#define MAX_DATA_SIZE 262144 // 1024 * 256;

/**
 * read the next chunk of bytes, create a node, and add a link to the node in the passed-in node
 * @param file the file handle
 * @param node the node to add to
 * @returns number of bytes read
 */
int ipfs_import_chunk(FILE* file, struct Node* node) {
	unsigned char buffer[MAX_DATA_SIZE];
	size_t bytes_read = fread(buffer, MAX_DATA_SIZE, 1, file);
	if (node->data_size == 0) {
		Node_Set_Data(node, buffer, bytes_read);
	} else {
		// create a new node, and link to the parent
		struct Node* new_node = N_Create_From_Data(buffer, bytes_read);
		// persist
		// put link in node
		Node_Add_Link(node, Create_Link("", new_node->cached->hash));
		Node_Delete(new_node);
	}
	return bytes_read;
}

/**
 * Creates a node based on an incoming file
 * @param file_name the file to import
 * @param node the root node (could have links to others)
 * @returns true(1) on success
 */
int ipfs_import_file(const char* fileName, struct Node** node) {
	int retVal = 1;

	FILE* file = fopen(fileName, "rb");
	*node = (struct Node)malloc(sizeof(struct Node));

	// add all nodes
	while (ipfs_import_chunk(file, *node) == MAX_DATA_SIZE) {}
	fclose(file);

	return 1;
}
