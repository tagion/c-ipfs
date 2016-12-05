#include "ipfs/merkledag/merkledag.h"
#include "ipfs/node/node.h"

int test_merkledag_add_data() {
	int retVal = 0;

	// open the fs repo
	struct RepoConfig* repo_config;
	struct FSRepo* fs_repo;
	const char* path = "/tmp/.ipfs";

	// create the struct
	retVal = ipfs_repo_fsrepo_new((char*)path, repo_config, &fs_repo);
	if (retVal == 0)
		return 0;

	// open the repository and read the config file
	retVal = ipfs_repo_fsrepo_open(fs_repo);
	if (retVal == 0) {
		ipfs_repo_fsrepo_free(fs_repo);
		return 0;
	}

	// create data for node
	size_t binary_data_size = 255;
	unsigned char binary_data[binary_data_size];
	for(int i = 0; i < binary_data_size; i++) {
		binary_data[i] = i;
	}

	// create a node
	struct Node* node = N_Create_From_Data(binary_data);

	retVal = ipfs_merkledag_add(node, fs_repo);
	if (retVal == 0)
		return 0;

	// make sure everything is correct
	if (node->cached == NULL)
		return 0;

	Node_Delete(node);

	return 1;
}
