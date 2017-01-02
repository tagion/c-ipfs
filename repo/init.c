#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include "ipfs/os/utils.h"
#include "ipfs/repo/config/config.h"
#include "ipfs/repo/fsrepo/fs_repo.h"

int make_ipfs_repository(const char* path) {
	int retVal;
	char currDirectory[1024];
	struct RepoConfig* repo_config;

	printf("initializing ipfs node at %s\n", path);
	// build a default repo config
	retVal = ipfs_repo_config_new(&repo_config);
	if (retVal == 0)
		return 0;
	printf("generating 2048-bit RSA keypair...");
	retVal = ipfs_repo_config_init(repo_config, 2048, path);
	if (retVal == 0)
		return 0;
	printf("done\n");
	// now the fs_repo
	struct FSRepo* fs_repo;
	retVal = ipfs_repo_fsrepo_new(path, repo_config, &fs_repo);
	if (retVal == 0)
		return 0;
	// this builds a new repo
	retVal = ipfs_repo_fsrepo_init(fs_repo);
	if (retVal == 0) {
		ipfs_repo_fsrepo_free(fs_repo);
		return 0;
	}

	// give some results to the user
	printf("peer identity: %s\n", fs_repo->config->identity->peer_id);

	// clean up
	ipfs_repo_fsrepo_free(fs_repo);

	// make sure the repository exists
	retVal = os_utils_filepath_join(path, "config", currDirectory, 1024);
	if (retVal == 0)
		return 0;
	retVal = os_utils_file_exists(currDirectory);
	return retVal;
}


/**
 * Initialize a repository
 */
int ipfs_repo_init(int argc, char** argv) {
	// the default is the user's home directory
	char* home_directory = os_utils_get_homedir();
	//allow user to pass directory on command line
	if (argc > 2) {
		home_directory = argv[2];
	} else {
		// check the IPFS_PATH
		char* temp = os_utils_getenv("IPFS_PATH");
		if (temp != NULL)
			home_directory = temp;
	}
	// get the full path
	int dir_len = strlen(home_directory) + 7;
	char dir[dir_len];
	strcpy(dir, home_directory);
	os_utils_filepath_join(home_directory, ".ipfs", dir, dir_len);
	// make sure it doesn't already exist
	if (os_utils_file_exists(dir)) {
		printf("Directory already exists: %s\n", dir);
		return 0;
	}
	// make the directory
	if (mkdir(dir, S_IRWXU) == -1) {
		printf("Unable to create the directory: %s\n", dir);
		return 0;
	}
	// make the repository
	return make_ipfs_repository(dir);
}
