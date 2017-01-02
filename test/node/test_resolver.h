#include "ipfs/importer/resolver.h"
#include "ipfs/os/utils.h"

int test_resolver_get() {
	// clean out repository
	const char* ipfs_path = "/tmp/.ipfs";
	os_utils_setenv("IPFS_PATH", ipfs_path, 1);

	drop_and_build_repository(ipfs_path);

	int argc = 4;
	char* argv[argc];
	argv[0] = "ipfs";
	argv[1] = "add";
	argv[2] = "-r";
	argv[3] = "/Users/JohnJones/ipfstest";

	ipfs_import_files(argc, (char**)argv);

	struct FSRepo* fs_repo;
	ipfs_repo_fsrepo_new(ipfs_path, NULL, &fs_repo);
	ipfs_repo_fsrepo_open(fs_repo);

	// find something that is already in the repository
	struct Node* result = ipfs_resolver_get("/ipfs/QmbMecmXESf96ZNry7hRuzaRkEBhjqXpoYfPCwgFzVGDzB", NULL, fs_repo);
	if (result == NULL) {
		return 0;
	}

	ipfs_node_free(result);

	// find something by path
	result = ipfs_resolver_get("/ipfs/QmZBvycPAYScBoPEzm35zXHt6gYYV5t9PyWmr4sksLPNFS/hello_world.txt", NULL, fs_repo);
	if (result == NULL) {
		return 0;
	}

	return 1;
}
