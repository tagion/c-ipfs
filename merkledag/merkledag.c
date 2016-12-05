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

	// turn the node into a block
	struct Block* block;
	ipfs_blocks_block_new(node->data, node->data_size, &block);

	int retVal = fs_repo->config->datastore->datastore_put(block->cid->hash, block->cid->hash_length, block->data, block->data_length, fs_repo->config->datastore);
	if (retVal == 0) {
		ipfs_blocks_block_free(block);
		return 0;
	}

	Node_Set_Cid(node, block->cid);
	ipfs_blocks_block_free(block);

	// TODO: call HasBlock (unsure why as yet)
	return 1;
}
