#include <pthread.h>

#include "libp2p/os/utils.h"
#include "multiaddr/multiaddr.h"
#include "ipfs/core/daemon.h"
#include "../test_helper.h"
#include "ipfs/routing/routing.h"

void* test_routing_daemon_start(void* arg) {
	ipfs_daemon_start((char*)arg);
	return NULL;
}

int test_routing_find_peer() {
	// clean out repository
	char* ipfs_path = "/tmp/test1";
	os_utils_setenv("IPFS_PATH", ipfs_path, 1);
	char* peer_id_1;
	char* peer_id_2;
	struct FSRepo* fs_repo_2;
	char* peer_id_3;
	pthread_t thread1;
	pthread_t thread2;
	struct MultiAddress* ma_peer1;

	// create peer 1
	drop_and_build_repository(ipfs_path, 4001, NULL, &peer_id_1);
	char multiaddress_string[255];
	sprintf(multiaddress_string, "/ip4/127.0.0.1/tcp/4001/ipfs/%s", peer_id_1);
	ma_peer1 = multiaddress_new_from_string(multiaddress_string);
	// start the daemon in a separate thread
	if (pthread_create(&thread1, NULL, test_routing_daemon_start, (void*)ipfs_path) < 0)
		return 0;

	// create peer 2
	ipfs_path = "/tmp/test2";
	os_utils_setenv("IPFS_PATH", ipfs_path, 1);
	struct Libp2pVector* ma_vector = libp2p_utils_vector_new(1);
	libp2p_utils_vector_add(ma_vector, ma_peer1);
	drop_and_build_repository(ipfs_path, 4002, ma_vector, &peer_id_2);
	// add a file, to prime the connection to peer 1
	//TODO: Find a better way to do this...
	size_t bytes_written = 0;
	ipfs_repo_fsrepo_new(ipfs_path, NULL, &fs_repo_2);
	ipfs_repo_fsrepo_open(fs_repo_2);
	struct Node* node = NULL;
	ipfs_import_file(NULL, "/home/parallels/ipfstest/hello_world.txt", &node, fs_repo_2, &bytes_written, 0);
	ipfs_repo_fsrepo_free(fs_repo_2);
	// start the daemon in a separate thread
	if (pthread_create(&thread2, NULL, test_routing_daemon_start, (void*)ipfs_path) < 0)
		return 0;

	// create my peer, peer 3
	ipfs_path = "/tmp/test3";
	os_utils_setenv("IPFS_PATH", ipfs_path, 1);
	drop_and_build_repository(ipfs_path, 4003, ma_vector, &peer_id_3);

	struct FSRepo* fs_repo;
	ipfs_repo_fsrepo_new(ipfs_path, NULL, &fs_repo);
	ipfs_repo_fsrepo_open(fs_repo);

	// We know peer 1, try to find peer 2
    struct IpfsNode local_node;
    local_node.mode = MODE_ONLINE;
    local_node.peerstore = libp2p_peerstore_new();
    local_node.repo = fs_repo;
    local_node.identity = fs_repo->config->identity;
    local_node.routing = ipfs_routing_new_online(&local_node, &fs_repo->config->identity->private_key, NULL);

    local_node.routing->Bootstrap(local_node.routing);

    struct Libp2pPeer* result;
    struct Libp2pPeer* first_peer = ((struct PeerEntry*)local_node.peerstore->head_entry->item)->peer;
    if (!local_node.routing->FindPeer(local_node.routing, peer_id_2, strlen(peer_id_2), &result))
    	return 0;

	if (result == NULL) {
		ipfs_repo_fsrepo_free(fs_repo);
		pthread_cancel(thread1);
		pthread_cancel(thread2);
		return 0;
	}

	struct MultiAddress *remote_address = result->addr_head->item;
	fprintf(stderr, "Remote address is: %s\n", remote_address->string);

	ipfs_repo_fsrepo_free(fs_repo);
	pthread_cancel(thread1);
	pthread_cancel(thread2);
	return 1;

}
