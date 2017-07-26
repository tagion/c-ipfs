#include <stdlib.h>
#include "libp2p/conn/session.h"
#include "libp2p/utils/vector.h"
#include "ipfs/exchange/bitswap/wantlist_queue.h"

/**
 * remove this session from the lists of sessions that are looking for this WantListQueueEntry
 * @param entry the entry
 * @param session who was looking for it
 * @returns true(1) on success, false(0) otherwise
 */
int ipfs_bitswap_wantlist_queue_entry_decrement(struct WantListQueueEntry* entry, const struct WantListSession* session) {
	for(size_t i = 0; i < entry->sessionsRequesting->total; i++) {
		const struct WantListSession* current = (const struct WantListSession*)libp2p_utils_vector_get(entry->sessionsRequesting, i);
		if (ipfs_bitswap_wantlist_session_compare(session, current) == 0) {
			libp2p_utils_vector_delete(entry->sessionsRequesting, i);
			return 1;
		}
	}
	return 0;
}


/***
 * Initialize a new Wantlist (there should only be 1 per instance)
 * @returns a new WantList
 */
struct WantListQueue* ipfs_bitswap_wantlist_queue_new() {
	struct WantListQueue* wantlist = (struct WantListQueue*) malloc(sizeof(struct WantListQueue));
	if (wantlist != NULL) {
		pthread_mutex_init(&wantlist->wantlist_mutex, NULL);
		wantlist->queue = NULL;
	}
	return wantlist;
}

/***
 * Deallocate resources of a WantList
 * @param wantlist the WantList
 * @returns true(1)
 */
int ipfs_bitswap_wantlist_queue_free(struct WantListQueue* wantlist) {
	if (wantlist != NULL) {
		if (wantlist->queue != NULL) {
			libp2p_utils_vector_free(wantlist->queue);
			wantlist->queue = NULL;
		}
		free(wantlist);
	}
	return 1;
}

/***
 * Add a Cid to the WantList
 * @param wantlist the WantList to add to
 * @param cid the Cid to add
 * @returns the correct WantListEntry or NULL if error
 */
struct WantListQueueEntry* ipfs_bitswap_wantlist_queue_add(struct WantListQueue* wantlist, const struct Cid* cid, const struct WantListSession* session) {
	struct WantListQueueEntry* entry = NULL;
	if (wantlist != NULL) {
		pthread_mutex_lock(&wantlist->wantlist_mutex);
		if (wantlist->queue == NULL) {
			wantlist->queue = libp2p_utils_vector_new(1);
		}
		entry = ipfs_bitswap_wantlist_queue_find(wantlist, cid);
		if (entry == NULL) {
			// create a new one
			entry = ipfs_bitswap_wantlist_queue_entry_new();
			entry->cid = ipfs_cid_copy(cid);
			entry->priority = 1;
			libp2p_utils_vector_add(wantlist->queue, entry);
		}
		libp2p_utils_vector_add(entry->sessionsRequesting, session);
		pthread_mutex_unlock(&wantlist->wantlist_mutex);
	}
	return entry;
}

/***
 * Remove (decrement the counter) a Cid from the WantList
 * @param wantlist the WantList
 * @param cid the Cid
 * @returns true(1) on success, otherwise false(0)
 */
int ipfs_bitswap_wantlist_queue_remove(struct WantListQueue* wantlist, const struct Cid* cid, const struct WantListSession* session) {
	//TODO: remove if counter is <= 0
	if (wantlist != NULL) {
		struct WantListQueueEntry* entry = ipfs_bitswap_wantlist_queue_find(wantlist, cid);
		if (entry != NULL) {
			ipfs_bitswap_wantlist_queue_entry_decrement(entry, session);
			return 1;
		}
	}
	return 0;
}

/***
 * Find a Cid in the WantList
 * @param wantlist the list
 * @param cid the Cid
 * @returns the WantListQueueEntry
 */
struct WantListQueueEntry* ipfs_bitswap_wantlist_queue_find(struct WantListQueue* wantlist, const struct Cid* cid) {
	for (size_t i = 0; i < wantlist->queue->total; i++) {
		struct WantListQueueEntry* entry = (struct WantListQueueEntry*) libp2p_utils_vector_get(wantlist->queue, i);
		if (entry == NULL) {
			//TODO: something went wrong. This should be logged.
			return NULL;
		}
		if (ipfs_cid_compare(cid, entry->cid) == 0) {
			return entry;
		}
	}
	return NULL;
}

/***
 * Initialize a WantListQueueEntry
 * @returns a new WantListQueueEntry
 */
struct WantListQueueEntry* ipfs_bitswap_wantlist_queue_entry_new() {
	struct WantListQueueEntry* entry = (struct WantListQueueEntry*) malloc(sizeof(struct WantListQueueEntry));
	if (entry != NULL) {
		entry->block = NULL;
		entry->cid = NULL;
		entry->priority = 0;
		entry->sessionsRequesting = NULL;
	}
	return entry;
}

/***
 * Free a WantListQueueENtry struct
 * @param entry the struct
 * @returns true(1)
 */
int ipfs_bitswap_wantlist_queue_entry_free(struct WantListQueueEntry* entry) {
	if (entry != NULL) {
		if (entry->block != NULL) {
			ipfs_block_free(entry->block);
			entry->block = NULL;
		}
		if (entry->cid != NULL) {
			ipfs_cid_free(entry->cid);
			entry->cid = NULL;
		}
		if (entry->sessionsRequesting != NULL) {
			libp2p_utils_vector_free(entry->sessionsRequesting);
			entry->sessionsRequesting = NULL;
		}
		free(entry);
	}
	return 1;
}

int ipfs_bitswap_wantlist_session_compare(const struct WantListSession* a, const struct WantListSession* b) {
	if (a == NULL && b == NULL)
		return 0;
	if (a == NULL && b != NULL)
		return -1;
	if (a != NULL && b == NULL)
		return 1;
	if (a->type != b->type)
		return b->type - a->type;
	if (a->type == WANTLIST_SESSION_TYPE_LOCAL) {
		// it's local, there should be only 1
		return 0;
	} else {
		struct SessionContext* contextA = (struct SessionContext*)a->context;
		struct SessionContext* contextB = (struct SessionContext*)b->context;
		return libp2p_session_context_compare(contextA, contextB);
	}
}

