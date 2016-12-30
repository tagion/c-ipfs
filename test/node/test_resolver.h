#include "ipfs/importer/resolver.h"

int test_resolver_get() {
	// clean out repository
	drop_and_build_repository("/Users/JohnJones/.ipfs");

	int argc = 3;
	char* argv[argc];
	argv[0] = "ipfs";
	argv[1] = "add";
	argv[2] = "/Users/JohnJones/ipfstest";

	ipfs_import_files(argc, (char**)argv);

	struct FSRepo* fs_repo;
	ipfs_repo_fsrepo_new("/Users/JohnJones/.ipfs", NULL, &fs_repo);
	ipfs_repo_fsrepo_open(fs_repo);

	// find something that is already in the repository
	struct Node* result = ipfs_resolver_get("/ipfs/QmbMecmXESf96ZNry7hRuzaRkEBhjqXpoYfPCwgFzVGDzB", NULL, fs_repo);
	if (result == NULL) {
		return 0;
	}

	ipfs_node_free(result);

	// find something by path
	result = ipfs_resolver_get("/ipfs/QmWKtXwRg4oL2KaXhvJ3KyGjFE2PVKREwu7qb65V7ficui/hello_world.txt", NULL, fs_repo);
	if (result == NULL) {
		return 0;
	}

	return 1;
}
