#include <stdio.h>
#include <string.h>

#include "ipfs/cid/cid.h"
#include "ipfs/merkledag/merkledag.h"
#include "ipfs/merkledag/node.h"
#include "ipfs/repo/fsrepo/fs_repo.h"
/**
 * pull objects from ipfs
 */

/**
 * get a file by its hash, and write the data to a file
 * @param hash the base58 multihash of the cid
 * @param file_name the file name to write to
 * @returns true(1) on success
 */
int ipfs_exporter_to_file(const unsigned char* hash, const char* file_name, const struct FSRepo* fs_repo) {
	// convert hash to cid
	struct Cid* cid = NULL;
	if ( ipfs_cid_decode_hash_from_base58(hash, strlen((char*)hash), &cid) == 0) {
		return 0;
	}

	// find block
	struct Node* read_node = NULL;
	if (ipfs_merkledag_get(cid, &read_node, fs_repo) == 0) {
		ipfs_cid_free(cid);
		return 0;
	}

	// no longer need the cid
	ipfs_cid_free(cid);

	// process blocks
	FILE* file = fopen(file_name, "wb");
	if (file == NULL) {
		ipfs_node_free(read_node);
		return 0;
	}
	size_t bytes_written = fwrite(read_node->data, 1, read_node->data_size, file);
	if (bytes_written != read_node->data_size) {
		fclose(file);
		ipfs_node_free(read_node);
		return 0;
	}
	struct NodeLink* link = read_node->head_link;
	struct Node* link_node = NULL;
	while (link != NULL) {
		if ( ipfs_merkledag_get(link->cid, &link_node, fs_repo) == 0) {
			fclose(file);
			ipfs_node_free(read_node);
			return 0;
		}
		bytes_written = fwrite(link_node->data, 1, link_node->data_size, file);
		if (bytes_written != link_node->data_size) {
			ipfs_node_free(link_node);
			fclose(file);
			ipfs_node_free(read_node);
			return 0;
		}
		ipfs_node_free(link_node);
		link = link->next;
	}

	fclose(file);

	if (read_node != NULL)
		ipfs_node_free(read_node);

	return 1;
}
