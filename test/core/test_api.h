#include <pthread.h>

#include "../test_helper.h"
#include "libp2p/utils/logger.h"
#include "ipfs/core/client_api.h"
#include "ipfs/core/daemon.h"
#include "ipfs/importer/exporter.h"
#include "ipfs/importer/importer.h"

int test_core_api_startup_shutdown() {
	char* repo_path = "/tmp/ipfs_1/.ipfs";
	char* peer_id = NULL;
	int retVal = 0;

	if (!drop_and_build_repository(repo_path, 4001, NULL, &peer_id))
		goto exit;

	// this should start the api
	test_daemon_start(repo_path);
	sleep(3);

	struct IpfsNode* client_node = NULL;
	if (!ipfs_node_offline_new(repo_path, &client_node)) {
		goto exit;
	}
	// test to see if it is working
	if (client_node->mode == MODE_API_AVAILABLE)
		goto exit;

	retVal = 1;
	// cleanup
	exit:
	ipfs_daemon_stop();
	if (peer_id != NULL)
		free(peer_id);

	return retVal;
}

/***
 * Attempt to get a file over the "network" using the api
 */
int test_core_api_object_cat() {
	int retVal = 0;
	pthread_t daemon_thread1;
	pthread_t daemon_thread2;
	int thread_started1 = 0;
	int thread_started2 = 0;
	char* ipfs_path1 = "/tmp/ipfs_1/.ipfs";
	char* config_file1 = "config.test1";
	char* ipfs_path2 = "/tmp/ipfs_2/.ipfs";
	char* config_file2 = "config.test2";
	struct FSRepo* fs_repo = NULL;
	char hash[256] = "";
	char* args[] = {"ipfs", "--config", ipfs_path2, "cat", hash };

	// logging
	libp2p_logger_add_class("test_journal");
	libp2p_logger_add_class("journal");
	libp2p_logger_add_class("daemon");
	libp2p_logger_add_class("online");
	libp2p_logger_add_class("peer");
	libp2p_logger_add_class("null");
	libp2p_logger_add_class("replication");
	libp2p_logger_add_class("fs_repo");
	libp2p_logger_add_class("lmdb_journalstore");
	libp2p_logger_add_class("lmdb_datastore");
	libp2p_logger_add_class("secio");
	libp2p_logger_add_class("socket");
	libp2p_logger_add_class("protocol");
	libp2p_logger_add_class("dht_protocol");
	libp2p_logger_add_class("resolver");
	libp2p_logger_add_class("unixfs");
	libp2p_logger_add_class("bitswap_engine");
	libp2p_logger_add_class("bitswap_network");

	// build 2 repos
	if (!drop_build_open_repo(ipfs_path1, &fs_repo, config_file1)) {
		ipfs_repo_fsrepo_free(fs_repo);
		libp2p_logger_error("test_api", "Unable to drop and build repository at %s\n", ipfs_path1);
		goto exit;
	}
	libp2p_logger_debug("test_api", "Changed the server id to %s.\n", fs_repo->config->identity->peer->id);
	ipfs_repo_fsrepo_free(fs_repo);
	if (!drop_build_open_repo(ipfs_path2, &fs_repo, config_file2)) {
		ipfs_repo_fsrepo_free(fs_repo);
		libp2p_logger_error("test_api", "Unable to drop and build repository at %s\n", ipfs_path2);
		goto exit;
	}
	libp2p_logger_debug("test_api", "Changed the server id to %s.\n", fs_repo->config->identity->peer->id);
	ipfs_repo_fsrepo_free(fs_repo);

	// add some files to the first repo
	uint8_t *bytes = (unsigned char*)"hello, world!\n";
	char* filename = "test1.txt";
	create_file(filename, bytes, strlen((char*)bytes));
	struct HashtableNode* node;
	size_t bytes_written;
	struct IpfsNode *local_node = NULL;
	ipfs_node_offline_new(ipfs_path1, &local_node);
	ipfs_import_file(NULL, filename, &node, local_node, &bytes_written, 0);
	memset(hash, 0, 256);
	ipfs_cid_hash_to_base58(node->hash, node->hash_size, (unsigned char*)hash, 256);
	ipfs_node_free(local_node);
	ipfs_hashtable_node_free(node);

	libp2p_logger_debug("test_api", "*** Firing up daemons ***\n");
	pthread_create(&daemon_thread1, NULL, test_daemon_start, (void*)ipfs_path1);
	thread_started1 = 1;
	pthread_create(&daemon_thread2, NULL, test_daemon_start, (void*)ipfs_path2);
	thread_started2 = 1;

	sleep(3);

	// use a client to ask for the file on server 1
	if (ipfs_exporter_object_cat(5, args) == 0) {
		goto exit;
	}

	retVal = 1;
	exit:
	ipfs_daemon_stop();
	if (thread_started1)
		pthread_join(daemon_thread1, NULL);
	if (thread_started2)
		pthread_join(daemon_thread2, NULL);
	return retVal;
}
