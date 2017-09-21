#include <stdlib.h>
#include <pthread.h>

#include "ipfs/cid/cid.h"
#include "ipfs/core/ipfs_node.h"
#include "ipfs/namesys/publisher.h"

int test_namesys_publisher_publish() {
	int retVal = 0;
	struct IpfsNode* local_node = NULL;
	struct Cid* cid = NULL;
	char* hash_text = "QmZtAEqmnXMZkwVPKdyMGxUoo35cQMzNhmq6CN3DvgRwAD";
	char* repo_path = "/tmp/ipfs_1/.ipfs";
	pthread_t api_pth = 0;

	// get a local node
	if (!ipfs_node_offline_new(repo_path, &local_node)) {
		libp2p_logger_error("test_publisher", "publish: Unable to open ipfs repository.\n");
		goto exit;
	}

	// get a cid
	if (!ipfs_cid_decode_hash_from_base58((unsigned char*)hash_text, strlen(hash_text), &cid)) {
		libp2p_logger_error("test_publisher", "publish: Unable to convert hash from base58.\n");
		goto exit;
	}

	// attempt to publish
	if (!ipfs_namesys_publisher_publish(local_node, cid)) {
		libp2p_logger_error("test_publisher", "publish: Unable to publish %s.\n", hash_text);
		goto exit;
	}

	retVal = 1;
	exit:
	ipfs_node_free(&api_pth, local_node);
	ipfs_cid_free(cid);
	return retVal;
}
