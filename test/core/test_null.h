#include <stdlib.h>

int test_null_add_provider() {
	int retVal = 0;
	char* peer_id_1;
	char* peer_id_2;
	struct FSRepo *fs_repo_2 = NULL;
	pthread_t thread1;
	pthread_t thread2;
	struct MultiAddress* ma_peer1;
	char* ipfs_path = "/tmp/test1";

	// create peer 1 that will be the "server" for this test
	os_utils_setenv("IPFS_PATH", ipfs_path, 1);
	drop_and_build_repository(ipfs_path, 4001, NULL, &peer_id_1);
	char multiaddress_string[255];
	sprintf(multiaddress_string, "/ip4/127.0.0.1/tcp/4001/ipfs/%s", peer_id_1);
	ma_peer1 = multiaddress_new_from_string(multiaddress_string);
	// start the daemon in a separate thread
	if (pthread_create(&thread1, NULL, test_routing_daemon_start, (void*)ipfs_path) < 0)
		goto exit;

	// create peer 2 that will be the "client" for this test
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
		goto exit;

    // wait for everything to start up
    // JMJ debugging
    sleep(60);

	//TODO: verify that the server (peer 1) has the client and his file

	retVal = 1;
	exit:
	if (fs_repo_2 != NULL)
		ipfs_repo_fsrepo_free(fs_repo_2);
	if (ma_peer1 != NULL)
		multiaddress_free(ma_peer1);
	pthread_cancel(thread1);
	pthread_cancel(thread2);
	return retVal;
}
