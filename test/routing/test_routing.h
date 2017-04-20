#include <pthread.h>

#include "libp2p/os/utils.h"
#include "libp2p/utils/logger.h"

#include "multiaddr/multiaddr.h"

#include "ipfs/core/daemon.h"
#include "ipfs/core/ipfs_node.h"
#include "ipfs/routing/routing.h"
#include "ipfs/importer/exporter.h"

#include "../test_helper.h"

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
	struct IpfsNode *local_node2;
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
	ipfs_node_online_new(ipfs_path, &local_node2);
	struct HashtableNode* node = NULL;
	ipfs_import_file(NULL, "/home/parallels/ipfstest/hello_world.txt", &node, local_node2, &bytes_written, 0);
	ipfs_node_free(local_node2);
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
    if (!local_node.routing->FindPeer(local_node.routing, (unsigned char*)peer_id_2, strlen(peer_id_2), &result))
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

int test_routing_find_providers() {
	int retVal = 0;
	// clean out repository
	char* ipfs_path = "/tmp/test1";
	os_utils_setenv("IPFS_PATH", ipfs_path, 1);
	char* peer_id_1 = NULL;
	char* peer_id_2 = NULL;
	struct IpfsNode *local_node2 = NULL;;
	char* peer_id_3 = NULL;
	char* remote_peer_id = NULL;
	pthread_t thread1, thread2;
	int thread1_started = 0, thread2_started = 0;
	struct MultiAddress* ma_peer1 = NULL;
	struct Libp2pVector* ma_vector2 = NULL, *ma_vector3 = NULL;
    struct IpfsNode local_node;
    struct FSRepo* fs_repo = NULL;

	// create peer 1
	drop_and_build_repository(ipfs_path, 4001, NULL, &peer_id_1);
	char multiaddress_string[255];
	sprintf(multiaddress_string, "/ip4/127.0.0.1/tcp/4001/ipfs/%s", peer_id_1);
	ma_peer1 = multiaddress_new_from_string(multiaddress_string);
	// start the daemon in a separate thread
	if (pthread_create(&thread1, NULL, test_routing_daemon_start, (void*)ipfs_path) < 0) {
		fprintf(stderr, "Unable to start thread 1\n");
		goto exit;
	}
	thread1_started = 1;

	// create peer 2
	ipfs_path = "/tmp/test2";
	os_utils_setenv("IPFS_PATH", ipfs_path, 1);
	// create a vector to hold peer1's multiaddress so we can connect as a peer
	ma_vector2 = libp2p_utils_vector_new(1);
	libp2p_utils_vector_add(ma_vector2, ma_peer1);
	// note: this distroys some things, as it frees the fs_repo:
	drop_and_build_repository(ipfs_path, 4002, ma_vector2, &peer_id_2);
	// add a file, to prime the connection to peer 1
	//TODO: Find a better way to do this...
	size_t bytes_written = 0;
	ipfs_node_online_new(ipfs_path, &local_node2);
	struct HashtableNode* node = NULL;
	ipfs_import_file(NULL, "/home/parallels/ipfstest/hello_world.txt", &node, local_node2, &bytes_written, 0);
	ipfs_node_free(local_node2);
	// start the daemon in a separate thread
	if (pthread_create(&thread2, NULL, test_routing_daemon_start, (void*)ipfs_path) < 0) {
		fprintf(stderr, "Unable to start thread 2\n");
		goto exit;
	}
	thread2_started = 1;

    // wait for everything to start up
    // JMJ debugging =
    sleep(3);

    // create my peer, peer 3
	ipfs_path = "/tmp/test3";
	os_utils_setenv("IPFS_PATH", ipfs_path, 1);
	ma_peer1 = multiaddress_new_from_string(multiaddress_string);
	ma_vector3 = libp2p_utils_vector_new(1);
	libp2p_utils_vector_add(ma_vector3, ma_peer1);
	drop_and_build_repository(ipfs_path, 4003, ma_vector3, &peer_id_3);

	ipfs_repo_fsrepo_new(ipfs_path, NULL, &fs_repo);
	ipfs_repo_fsrepo_open(fs_repo);

	// We know peer 1, try to find peer 2
    local_node.mode = MODE_ONLINE;
    local_node.peerstore = libp2p_peerstore_new();
    local_node.providerstore = libp2p_providerstore_new();
    local_node.repo = fs_repo;
    local_node.identity = fs_repo->config->identity;
    local_node.routing = ipfs_routing_new_online(&local_node, &fs_repo->config->identity->private_key, NULL);

    local_node.routing->Bootstrap(local_node.routing);

    struct Libp2pVector* result;
    if (!local_node.routing->FindProviders(local_node.routing, node->hash, node->hash_size, &result)) {
    	fprintf(stderr, "Unable to find a provider\n");
    	goto exit;
    }

	if (result == NULL) {
		fprintf(stderr, "Provider array is NULL\n");
		goto exit;
	}

	// connect to peer 2
	struct Libp2pPeer *remote_peer = NULL;
	for(int i = 0; i < result->total; i++) {
		remote_peer = libp2p_utils_vector_get(result, i);
		if (remote_peer->connection_type == CONNECTION_TYPE_CONNECTED || libp2p_peer_connect(remote_peer)) {
			break;
		}
		remote_peer = NULL;
	}

	if (remote_peer == NULL) {
		fprintf(stderr, "Remote Peer is NULL\n");
		goto exit;
	}

	remote_peer_id = malloc(remote_peer->id_size + 1);
	memcpy(remote_peer_id, remote_peer->id, remote_peer->id_size);
	remote_peer_id[remote_peer->id_size] = 0;
	fprintf(stderr, "Remote address is: %s\n", remote_peer_id);
	free(remote_peer_id);
	remote_peer_id = NULL;

	retVal = 1;
	exit:
	ipfs_daemon_stop();
	if (fs_repo != NULL)
		ipfs_repo_fsrepo_free(fs_repo);
	if (peer_id_1 != NULL)
		free(peer_id_1);
	if (peer_id_2 != NULL)
		free(peer_id_2);
	if (peer_id_3 != NULL)
		free(peer_id_3);
	if (thread1_started)
		pthread_join(thread1, NULL);
	if (thread2_started)
		pthread_join(thread2, NULL);
	if (ma_vector2 != NULL) {
		libp2p_utils_vector_free(ma_vector2);
	}
	if (ma_vector3 != NULL) {
		libp2p_utils_vector_free(ma_vector3);
	}
	if (local_node.providerstore != NULL)
		libp2p_providerstore_free(local_node.providerstore);
	if (local_node.peerstore != NULL) {
		libp2p_peerstore_free(local_node.peerstore);
	}
	if (local_node.routing != NULL) {
		ipfs_routing_online_free(local_node.routing);
	}
	if (node != NULL)
		ipfs_hashtable_node_free(node);
	if (result != NULL) {
		// we have a vector of peers. Clean 'em up:
		for(int i = 0; i < result->total; i++) {
			struct Libp2pPeer* p = (struct Libp2pPeer*)libp2p_utils_vector_get(result, i);
			libp2p_peer_free(p);
		}
		libp2p_utils_vector_free(result);
	}
	libp2p_logger_free();
	return retVal;

}

/***
 * Fire up a "client" and "server" and let the client tell the server he's providing a file
 */
int test_routing_provide() {
	int retVal = 0;
	// clean out repository
	char* ipfs_path = "/tmp/test1";
	os_utils_setenv("IPFS_PATH", ipfs_path, 1);
	char* peer_id_1 = NULL;
	char* peer_id_2 = NULL;
	struct IpfsNode *local_node2 = NULL;
	pthread_t thread1, thread2;
	int thread1_started = 0, thread2_started = 0;
	struct MultiAddress* ma_peer1 = NULL;
	struct Libp2pVector* ma_vector2 = NULL;

	// create peer 1
	drop_and_build_repository(ipfs_path, 4001, NULL, &peer_id_1);
	char multiaddress_string[255];
	sprintf(multiaddress_string, "/ip4/127.0.0.1/tcp/4001/ipfs/%s", peer_id_1);
	ma_peer1 = multiaddress_new_from_string(multiaddress_string);
	// start the daemon in a separate thread
	if (pthread_create(&thread1, NULL, test_routing_daemon_start, (void*)ipfs_path) < 0) {
		fprintf(stderr, "Unable to start thread 1\n");
		goto exit;
	}
	thread1_started = 1;

	// create peer 2
	ipfs_path = "/tmp/test2";
	os_utils_setenv("IPFS_PATH", ipfs_path, 1);
	// create a vector to hold peer1's multiaddress so we can connect as a peer
	ma_vector2 = libp2p_utils_vector_new(1);
	libp2p_utils_vector_add(ma_vector2, ma_peer1);
	// note: this distroys some things, as it frees the fs_repo:
	drop_and_build_repository(ipfs_path, 4002, ma_vector2, &peer_id_2);
	// add a file, to prime the connection to peer 1
	//TODO: Find a better way to do this...
	size_t bytes_written = 0;
	ipfs_node_online_new(ipfs_path, &local_node2);
	struct HashtableNode* node = NULL;
	ipfs_import_file(NULL, "/home/parallels/ipfstest/hello_world.txt", &node, local_node2, &bytes_written, 0);
	ipfs_node_free(local_node2);
	// start the daemon in a separate thread
	if (pthread_create(&thread2, NULL, test_routing_daemon_start, (void*)ipfs_path) < 0) {
		fprintf(stderr, "Unable to start thread 2\n");
		goto exit;
	}
	thread2_started = 1;

    // wait for everything to start up
    // JMJ debugging =
    sleep(3);

	retVal = 1;
	exit:
	ipfs_daemon_stop();
	if (peer_id_1 != NULL)
		free(peer_id_1);
	if (peer_id_2 != NULL)
		free(peer_id_2);
	if (thread1_started)
		pthread_join(thread1, NULL);
	if (thread2_started)
		pthread_join(thread2, NULL);
	if (ma_vector2 != NULL) {
		libp2p_utils_vector_free(ma_vector2);
	}
	if (node != NULL)
		ipfs_hashtable_node_free(node);
	libp2p_logger_free();
	return retVal;

}

/***
 * Attempt to retrieve a file from a previously unknown node
 */
int test_routing_retrieve_file_third_party() {
	int retVal = 0;

	/*
	libp2p_logger_add_class("online");
	libp2p_logger_add_class("multistream");
	libp2p_logger_add_class("null");
	libp2p_logger_add_class("dht_protocol");
	libp2p_logger_add_class("providerstore");
	libp2p_logger_add_class("peerstore");
	libp2p_logger_add_class("exporter");
	libp2p_logger_add_class("peer");
	libp2p_logger_add_class("test_routing");
	*/

	// clean out repository
	char* ipfs_path = "/tmp/test1";
	char* peer_id_1 = NULL, *peer_id_2 = NULL, *peer_id_3 = NULL;
	struct IpfsNode* ipfs_node2 = NULL, *ipfs_node3 = NULL;
	pthread_t thread1, thread2;
	int thread1_started = 0, thread2_started = 0;
	struct MultiAddress* ma_peer1 = NULL;
	struct Libp2pVector* ma_vector2 = NULL, *ma_vector3 = NULL;
	struct HashtableNode* node = NULL, *result_node = NULL;

	// create peer 1
	drop_and_build_repository(ipfs_path, 4001, NULL, &peer_id_1);
	char multiaddress_string[255];
	sprintf(multiaddress_string, "/ip4/127.0.0.1/tcp/4001/ipfs/%s", peer_id_1);
	ma_peer1 = multiaddress_new_from_string(multiaddress_string);
	// start the daemon in a separate thread
	libp2p_logger_debug("test_routing", "Firing up daemon 1.\n");
	if (pthread_create(&thread1, NULL, test_routing_daemon_start, (void*)ipfs_path) < 0) {
		fprintf(stderr, "Unable to start thread 1\n");
		goto exit;
	}
	thread1_started = 1;

    // wait for everything to start up
    // JMJ debugging =
    sleep(3);

    // create peer 2
	ipfs_path = "/tmp/test2";
	// create a vector to hold peer1's multiaddress so we can connect as a peer
	ma_vector2 = libp2p_utils_vector_new(1);
	libp2p_utils_vector_add(ma_vector2, ma_peer1);
	// note: this distroys some things, as it frees the fs_repo_3:
	drop_and_build_repository(ipfs_path, 4002, ma_vector2, &peer_id_2);
	// add a file, to prime the connection to peer 1
	//TODO: Find a better way to do this...
	size_t bytes_written = 0;
	if (!ipfs_node_online_new(ipfs_path, &ipfs_node2))
		goto exit;
	ipfs_node2->routing->Bootstrap(ipfs_node2->routing);
	ipfs_import_file(NULL, "/home/parallels/ipfstest/hello_world.txt", &node, ipfs_node2, &bytes_written, 0);
	ipfs_node_free(ipfs_node2);
	// start the daemon in a separate thread
	libp2p_logger_debug("test_routing", "Firing up daemon 2.\n");
	if (pthread_create(&thread2, NULL, test_routing_daemon_start, (void*)ipfs_path) < 0) {
		fprintf(stderr, "Unable to start thread 2\n");
		goto exit;
	}
	thread2_started = 1;

    // wait for everything to start up
    // JMJ debugging =
    sleep(3);

    libp2p_logger_debug("test_routing", "Firing up the 3rd client\n");
    // create my peer, peer 3
	ipfs_path = "/tmp/test3";
	ma_peer1 = multiaddress_new_from_string(multiaddress_string);
	ma_vector3 = libp2p_utils_vector_new(1);
	libp2p_utils_vector_add(ma_vector3, ma_peer1);
	drop_and_build_repository(ipfs_path, 4003, ma_vector3, &peer_id_3);
	ipfs_node_online_new(ipfs_path, &ipfs_node3);

    ipfs_node3->routing->Bootstrap(ipfs_node3->routing);

    if (!ipfs_exporter_get_node(ipfs_node3, node->hash, node->hash_size, &result_node)) {
    	fprintf(stderr, "Get_Node returned false\n");
    	goto exit;
    }

    if (node->hash_size != result_node->hash_size) {
    	fprintf(stderr, "Node hash sizes do not match. Should be %lu but is %lu\n", node->hash_size, result_node->hash_size);
    	goto exit;
    }

    if (node->data_size != result_node->data_size) {
    	fprintf(stderr, "Result sizes do not match. Should be %lu but is %lu\n", node->data_size, result_node->data_size);
    	goto exit;
    }

	retVal = 1;
	exit:
	ipfs_daemon_stop();
	if (thread1_started)
		pthread_join(thread1, NULL);
	if (thread2_started)
		pthread_join(thread2, NULL);
	if (ipfs_node3 != NULL)
		ipfs_node_free(ipfs_node3);
	if (peer_id_1 != NULL)
		free(peer_id_1);
	if (peer_id_2 != NULL)
		free(peer_id_2);
	if (peer_id_3 != NULL)
		free(peer_id_3);
	if (ma_vector2 != NULL) {
		libp2p_utils_vector_free(ma_vector2);
	}
	if (ma_vector3 != NULL) {
		libp2p_utils_vector_free(ma_vector3);
	}
	if (node != NULL)
		ipfs_hashtable_node_free(node);
	if (result_node != NULL)
		ipfs_hashtable_node_free(result_node);
	libp2p_logger_free();
	return retVal;

}
