#include <pthread.h>

#include "ipfs/importer/resolver.h"
#include "ipfs/os/utils.h"
#include "multiaddr/multiaddr.h"
#include "ipfs/core/daemon.h"

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

	struct IpfsNode ipfs_node;
	ipfs_node.repo = fs_repo;

	// find something that is already in the repository
	struct Node* result = ipfs_resolver_get("QmbMecmXESf96ZNry7hRuzaRkEBhjqXpoYfPCwgFzVGDzB", NULL, &ipfs_node);
	if (result == NULL) {
		free(test_dir);
		ipfs_repo_fsrepo_free(fs_repo);
		return 0;
	}

	ipfs_node_free(result);

	// find something where path includes the local node
	char path[255];
	strcpy(path, "/ipfs/");
	strcat(path, fs_repo->config->identity->peer_id);
	strcat(path, "/QmbMecmXESf96ZNry7hRuzaRkEBhjqXpoYfPCwgFzVGDzB");
	result = ipfs_resolver_get(path, NULL, &ipfs_node);
	if (result == NULL) {
		free(test_dir);
		ipfs_repo_fsrepo_free(fs_repo);
		return 0;
	}
	ipfs_node_free(result);

	// find something by path
	result = ipfs_resolver_get("QmZBvycPAYScBoPEzm35zXHt6gYYV5t9PyWmr4sksLPNFS/hello_world.txt", NULL, &ipfs_node);
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

void* test_resolver_daemon_start(void* arg) {
	ipfs_daemon_start((char*)arg);
	return NULL;
}

int test_resolver_remote_get() {
	// clean out repository
	const char* ipfs_path = "/tmp/.ipfs";
	os_utils_setenv("IPFS_PATH", ipfs_path, 1);
	char remote_peer_id[255];
	char path[255];
	drop_and_build_repository(ipfs_path);

	// start the daemon in a separate thread
	pthread_t thread;
	int rc = pthread_create(&thread, NULL, test_resolver_daemon_start, (void*)ipfs_path);

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

	// put the server in the peer store and change our peer id so we think it is remote (hack for now)
	strcpy(remote_peer_id, fs_repo->config->identity->peer_id);
	struct MultiAddress* remote_addr = multiaddress_new_from_string("/ip4/127.0.0.1/tcp/4001");
	struct Peerstore* peerstore = libp2p_peerstore_new();
	struct Libp2pPeer* peer = libp2p_peer_new_from_data(remote_peer_id, strlen(remote_peer_id), remote_addr);
	libp2p_peerstore_add_peer(peerstore, peer);
	strcpy(fs_repo->config->identity->peer_id, "QmABCD");

    struct IpfsNode local_node;
    local_node.mode = MODE_ONLINE;
    local_node.peerstore = peerstore;
    local_node.repo = fs_repo;
    local_node.identity = fs_repo->config->identity;

	// find something by remote path
	strcpy(path, "/ipfs/");
	strcat(path, remote_peer_id);
	strcat(path, "/QmZBvycPAYScBoPEzm35zXHt6gYYV5t9PyWmr4sksLPNFS/hello_world.txt");
	struct Node* result = ipfs_resolver_get(path, NULL, &local_node);
	if (result == NULL) {
		ipfs_repo_fsrepo_free(fs_repo);
		pthread_cancel(thread);
		return 0;
	}

	ipfs_node_free(result);
	ipfs_repo_fsrepo_free(fs_repo);
	pthread_cancel(thread);
	return 1;

}
