#include "ipfs/repo/fsrepo/fs_repo.h"
#include "../test_helper.h"

int test_repo_fsrepo_open_config() {
	struct FSRepo* fs_repo = NULL;
	struct RepoConfig* repo_config = NULL;

	const char* path = "/tmp/.ipfs";

	// create the struct
	int retVal = ipfs_repo_fsrepo_new((char*)path, repo_config, &fs_repo);
	if (retVal == 0)
		return 0;

	// open the repository and read the file
	retVal = ipfs_repo_fsrepo_open(fs_repo);
	if (retVal == 0) {
		ipfs_repo_fsrepo_free(fs_repo);
		return 0;
	}

	retVal = ipfs_repo_fsrepo_free(fs_repo);
	if (retVal == 0)
		return 0;

	return 1;
}

int test_repo_fsrepo_write_read_block() {
	struct Block* block = NULL;
	struct FSRepo* fs_repo = NULL;
	int retVal = 0;

	// freshen the repository
	retVal = drop_build_and_open_repo("/tmp/.ipfs", &fs_repo);
	if (retVal == 0)
		return 0;

	// make some data
	size_t data_size = 100;
	unsigned char data[data_size];

	int counter = 0;
	for(int i = 0; i < data_size; i++) {
		data[i] = counter++;
		if (counter > 15)
			counter = 0;
	}

	// create and write the block
	retVal = ipfs_blocks_block_new(&block);
	if (retVal == 0) {
		ipfs_repo_fsrepo_free(fs_repo);
		return 0;
	}
	retVal = ipfs_blocks_block_add_data(data, data_size, block);
	if (retVal == 0) {
		ipfs_repo_fsrepo_free(fs_repo);
		return 0;
	}

	retVal = ipfs_repo_fsrepo_block_write(block, fs_repo);
	if (retVal == 0) {
		ipfs_repo_fsrepo_free(fs_repo);
		ipfs_blocks_block_free(block);
		return 0;
	}

	// retrieve the block
	struct Block* results;
	retVal = ipfs_repo_fsrepo_block_read(block->cid, &results, fs_repo);
	if (retVal == 0) {
		ipfs_repo_fsrepo_free(fs_repo);
		ipfs_blocks_block_free(block);
		return 0;
	}

	// compare the two blocks
	retVal = 1;
	if (block->data_length != results->data_length || block->data_length != data_size) {
		printf("block data is of different length: %lu vs %lu\n", results->data_length, block->data_length);
		retVal = 0;
	}

	for(size_t i = 0; i < block->data_length; i++) {
		if (block->data[i] != results->data[i]) {
			printf("Data is different at position %lu. Should be %02x but is %02x\n", i, block->data[i], results->data[i]);
			retVal = 0;
			break;
		}
	}

	ipfs_repo_fsrepo_free(fs_repo);
	ipfs_blocks_block_free(block);
	ipfs_blocks_block_free(results);
	return retVal;
}
