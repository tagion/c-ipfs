#include "ipfs/core/api.h"
#include "libp2p/utils/logger.h"

int test_core_api_startup_shutdown() {
	struct IpfsNode* local_node = NULL;
	char* repo_path = "/tmp/ipfs_1";
	char* peer_id = NULL;
	int retVal = 0;

	if (!drop_and_build_repository(repo_path, 4001, NULL, &peer_id))
		goto exit;

	// this should start the api
	if (!ipfs_node_online_new(repo_path, &local_node))
		goto exit;


	// TODO: test to see if it works

	// TODO shut down
	retVal = 1;
	exit:

	return retVal;
}
