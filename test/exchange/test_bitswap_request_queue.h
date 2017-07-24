#include <stdlib.h>
#include "ipfs/exchange/bitswap/peer_request_queue.h"

/***
 * Create a queue, do some work, free the queue, make sure valgrind likes it.
 */
int test_bitswap_peer_request_queue_new() {
	// create a queue
	struct PeerRequestQueue* queue = ipfs_bitswap_peer_request_queue_new();
	struct PeerRequest* request = ipfs_bitswap_peer_request_new();
	ipfs_bitswap_peer_request_queue_add(queue, request);
	// clean up
	ipfs_bitswap_peer_request_queue_free(queue);
	return 1;
}
