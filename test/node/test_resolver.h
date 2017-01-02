#include "ipfs/importer/resolver.h"
#include "ipfs/os/utils.h"

int test_resolver_get() {
	// clean out repository
	const char* ipfs_path = "/tmp/.ipfs";
	os_utils_setenv("IPFS_PATH", ipfs_path, 1);

	drop_and_build_repository(ipfs_path);

	// this should point to a test directory with files and directories
	char* home_dir = os_utils_get_homedir();
	char* test_dir = malloc(strlen(home_dir) + 10);

	os_utils_filepath_join(home_dir, "ipfstest", test_dir, strlen(home_dir) + 10);

	int argc = 4;
	char* argv[argc];
	argv[0] = "ipfs";
	argv[1] = "add";
	argv[2] = "-r";
	argv[3] = test_dir;

	ipfs_import_files(argc, (char**)argv);

	struct FSRepo* fs_repo;
	ipfs_repo_fsrepo_new(ipfs_path, NULL, &fs_repo);
	ipfs_repo_fsrepo_open(fs_repo);

	// find something that is already in the repository
	struct Node* result = ipfs_resolver_get("/ipfs/QmbMecmXESf96ZNry7hRuzaRkEBhjqXpoYfPCwgFzVGDzB", NULL, fs_repo);
	if (result == NULL) {
		free(test_dir);
		ipfs_repo_fsrepo_free(fs_repo);
		return 0;
	}

	ipfs_node_free(result);

	// find something by path
	result = ipfs_resolver_get("/ipfs/QmZBvycPAYScBoPEzm35zXHt6gYYV5t9PyWmr4sksLPNFS/hello_world.txt", NULL, fs_repo);
	if (result == NULL) {
		free(test_dir);
		ipfs_repo_fsrepo_free(fs_repo);
		return 0;
	}

	ipfs_node_free(result);
	free(test_dir);
	ipfs_repo_fsrepo_free(fs_repo);

	return 1;
}
