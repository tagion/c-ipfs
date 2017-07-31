#pragma once
/***
 * A queue for requests to/from remote peers
 * NOTE: This must handle multiple threads
 */

#include <pthread.h>
#include "libp2p/peer/peer.h"
#include "ipfs/exchange/bitswap/bitswap.h"
#include "ipfs/blocks/block.h"

struct PeerRequest {
	pthread_mutex_t request_mutex;
	struct Libp2pPeer* peer;
	struct Libp2pVector* cids;
	struct Libp2pVector* blocks;
};

struct PeerRequestEntry {
	struct PeerRequestEntry* prior;
	struct PeerRequest* current;
	struct PeerRequestEntry* next;
};

struct PeerRequestQueue {
	pthread_mutex_t queue_mutex;
	struct PeerRequestEntry* first;
	struct PeerRequestEntry* last;
};

/**
 * Allocate resources for a new PeerRequest
 * @returns a new PeerRequest struct or NULL if there was a problem
 */
struct PeerRequest* ipfs_bitswap_peer_request_new();

/**
 * Free resources from a PeerRequest
 * @param request the request to free
 * @returns true(1)
 */
int ipfs_bitswap_peer_request_free(struct PeerRequest* request);

/**
 * Allocate resources for a new queue
 * @returns a new PeerRequestQueue
 */
struct PeerRequestQueue* ipfs_bitswap_peer_request_queue_new();

/**
 * Free all resources related to the queue
 * @param queue the queue
 * @returns true(1)
 */
int ipfs_bitswap_peer_request_queue_free(struct PeerRequestQueue* queue);

/**
 * Adds a peer request to the end of the queue
 * @param queue the queue
 * @param request the request
 * @returns true(1) on success, otherwise false
 */
int ipfs_bitswap_peer_request_queue_add(struct PeerRequestQueue* queue, struct PeerRequest* request);

/**
 * Removes a peer request from the queue, no mather where it is
 * @param queue the queue
 * @param request the request
 * @returns true(1) on success, otherwise false(0)
 */
int ipfs_bitswap_peer_request_quque_remove(struct PeerRequestQueue* queue, struct PeerRequest* request);

/**
 * Pull a PeerRequest off the queue
 * @param queue the queue
 * @returns the PeerRequest that should be handled next.
 */
struct PeerRequest* ipfs_bitswap_peer_request_queue_pop(struct PeerRequestQueue* queue);

/**
 * Finds a PeerRequestEntry that contains the specified PeerRequest
 * @param queue the queue
 * @param request what we're looking for
 * @returns the PeerRequestEntry or NULL if not found
 */
struct PeerRequestEntry* ipfs_bitswap_peer_request_queue_find_entry(struct PeerRequestQueue* queue, struct PeerRequest* request);

/***
 * Add a block to the appropriate peer's queue
 * @param queue the queue
 * @param who the session context that identifies the peer
 * @param block the block
 * @returns true(1) on success, otherwise false(0)
 */
int ipfs_bitswap_peer_request_queue_fill(struct PeerRequestQueue* queue, struct SessionContext* who, struct Block* block);

/***
 * Allocate resources for a PeerRequestEntry struct
 * @returns the allocated struct or NULL if there was a problem
 */
struct PeerRequestEntry* ipfs_bitswap_peer_request_entry_new();

/**
 * Frees resources allocated
 * @param entry the PeerRequestEntry to free
 * @returns true(1)
 */
int ipfs_bitswap_peer_request_entry_free(struct PeerRequestEntry* entry);

/****
 * Handle a PeerRequest
 * @param context the BitswapContext
 * @param request the request to process
 * @returns true(1) on succes, otherwise false(0)
 */
int ipfs_bitswap_peer_request_process_entry(const struct BitswapContext* context, struct PeerRequest* request);

/***
 * Find a PeerRequest related to a peer. If one is not found, it is created.
 *
 * @param peer_request_queue the queue to look through
 * @param peer the peer to look for
 * @returns a PeerRequestEntry or NULL on error
 */
struct PeerRequest* ipfs_peer_request_queue_find_peer(struct PeerRequestQueue* queue, struct Libp2pPeer* peer);

