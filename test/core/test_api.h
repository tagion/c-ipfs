#include "../test_helper.h"
#include "libp2p/utils/logger.h"
#include "ipfs/core/client_api.h"
#include "ipfs/core/daemon.h"

int test_core_api_startup_shutdown() {
	char* repo_path = "/tmp/ipfs_1";
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

int test_core_api_local_object_cat() {
	// add a file to the store, then use the api to get it
	return 0;
}
