#pragma once
/**
 * This is a list of requests from a peer (including locally).
 * NOTE: This tracks who wants what. If 2 peers want the same file,
 * there will be 1 WantListEntry in the WantList. There will be 2 entries in
 * WantListEntry.sessionsRequesting.
 */
#include <pthread.h>
#include "ipfs/cid/cid.h"
#include "ipfs/blocks/block.h"

enum WantListSessionType { WANTLIST_SESSION_TYPE_LOCAL, WANTLIST_SESSION_TYPE_REMOTE };

struct WantListSession {
	enum WantListSessionType type;
	void* context;
};

struct WantListQueueEntry {
	struct Cid* cid;
	int priority;
	// a vector of WantListSessions
	struct Libp2pVector* sessionsRequesting;
	struct Block* block;
};

struct WantListQueue {
	pthread_mutex_t wantlist_mutex;
	// a vector of WantListEntries
	struct Libp2pVector* queue;
};

/***
 * Initialize a WantListQueueEntry
 * @returns a new WantListQueueEntry
 */
struct WantListQueueEntry* ipfs_bitswap_wantlist_queue_entry_new();

/***
 * Remove resources, freeing a WantListQueueEntry
 * @param entry the WantListQueueEntry
 * @returns true(1)
 */
int ipfs_bitswap_wantlist_queue_entry_free(struct WantListQueueEntry* entry);

/***
 * Initialize a new Wantlist (there should only be 1 per instance)
 * @returns a new WantList
 */
struct WantListQueue* ipfs_bitswap_wantlist_queue_new();

/***
 * Deallocate resources of a WantList
 * @param wantlist the WantList
 * @returns true(1)
 */
int ipfs_bitswap_wantlist_queue_free(struct WantListQueue* wantlist);

/***
 * Add a Cid to the WantList
 * @param wantlist the WantList to add to
 * @param cid the Cid to add
 * @returns the correct WantListEntry or NULL if error
 */
struct WantListQueueEntry* ipfs_bitswap_wantlist_queue_add(struct WantListQueue* wantlist, const struct Cid* cid);

/***
 * Remove (decrement the counter) a Cid from the WantList
 * @param wantlist the WantList
 * @param cid the Cid
 * @returns true(1) on success, otherwise false(0)
 */
int ipfs_bitswap_wantlist_queue_remove(struct WantListQueue* wantlist, const struct Cid* cid);

/***
 * Find a Cid in the WantList
 * @param wantlist the list
 * @param cid the Cid
 * @returns the WantListQueueEntry
 */
struct WantListQueueEntry* ipfs_bitswap_wantlist_queue_find(struct WantListQueue* wantlist, const struct Cid* cid);


