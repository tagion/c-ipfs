/**
 * A basic storage building block of the IPFS system
 */

#include "ipfs/merkledag/merkledag.h"

/***
 * Adds a node to the dagService and blockService
 * @param node the node to add
 * @param cid the resultant cid that was added
 * @returns true(1) on success
 */
int ipfs_merkledag_add(struct Node* node, struct FSRepo* fs_repo) {
	// taken from merkledag.go line 59

	struct Block* block = NULL;

	// protobuf the node
	size_t protobuf_len = ipfs_node_protobuf_encode_size(node);
	size_t bytes_written = 0;
	unsigned char protobuf[protobuf_len];
	ipfs_node_protobuf_encode(node, protobuf, protobuf_len, &bytes_written);

	// turn the node into a block
	ipfs_blocks_block_new(&block);
	ipfs_blocks_block_add_data(protobuf, bytes_written, block);

	// write to block store & datastore
	int retVal = ipfs_repo_fsrepo_block_write(block, fs_repo);
	if (retVal == 0) {
		ipfs_blocks_block_free(block);
		return 0;
	}

	ipfs_node_set_cached(node, block->cid);

	if (block != NULL)
		ipfs_blocks_block_free(block);

	// TODO: call HasBlock (unsure why as yet)
	return 1;
}

/***
 * Retrieves a node from the datastore based on the cid
 * @param cid the key to look for
 * @param node the node to be created
 * @param fs_repo the repository
 * @returns true(1) on success
 */
int ipfs_merkledag_get(const struct Cid* cid, struct Node** node, const struct FSRepo* fs_repo) {
	int retVal = 1;
	struct Block* block;
	size_t key_length = 100;
	unsigned char key[key_length];

	// look for the node in the datastore. If it is not there, it is not a node.
	// If it exists, it is only a block.
	retVal = fs_repo->config->datastore->datastore_get((char*)cid->hash, cid->hash_length, key, key_length, &key_length, fs_repo->config->datastore);
	if (retVal == 0)
		return 0;

	// we have the record from the db. Go get the block from the blockstore
	retVal = ipfs_repo_fsrepo_block_read(cid, &block, fs_repo);
	if (retVal == 0) {
		return 0;
	}

	// now convert the block into a node
	if (ipfs_node_protobuf_decode(block->data, block->data_length, node) == 0) {
		ipfs_blocks_block_free(block);
		return 0;
	}

	// set the cid on the node
	if (ipfs_node_set_cached(*node, cid) == 0) {
		ipfs_blocks_block_free(block);
		ipfs_node_free(*node);
		return 0;
	}

	// free resources
	ipfs_blocks_block_free(block);

	return 1;
}
