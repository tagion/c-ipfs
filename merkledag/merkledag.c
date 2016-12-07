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

	int retVal = fs_repo->config->datastore->datastore_put_block(block, fs_repo->config->datastore);
	if (retVal == 0) {
		ipfs_blocks_block_free(block);
		return 0;
	}

	Node_Set_Cached(node, block->cid);
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

	// look for the block
	struct Block* block;
	retVal = fs_repo->config->datastore->datastore_get_block(cid, &block, fs_repo->config->datastore);
	if (retVal == 0)
		return 0;

	// we have the block. Fill the node
	*node = N_Create_From_Data(block->data, block->data_length);
	Node_Set_Cached(*node, cid);

	return retVal;
}
