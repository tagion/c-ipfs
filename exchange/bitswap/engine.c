#include <unistd.h>
#include "ipfs/exchange/bitswap/engine.h"
#include "ipfs/exchange/bitswap/wantlist_queue.h"
#include "ipfs/exchange/bitswap/peer_request_queue.h"

/***
 * Implementation of the bitswap engine
 */

/***
 * Allocate resources for a BitswapEngine
 * @returns a new struct BitswapEngine
 */
struct BitswapEngine* ipfs_bitswap_engine_new() {
	struct BitswapEngine* engine = (struct BitswapEngine*) malloc(sizeof(struct BitswapEngine));
	if (engine != NULL) {
		engine->shutting_down = 0;
	}
	return engine;
}

/***
 * Deallocate resources from struct BitswapEngine
 * @param engine the engine to free
 * @returns true(1)
 */
int ipfs_bitswap_engine_free(struct BitswapEngine* engine) {
	free(engine);
	return 1;
}

/***
 * A separate thread that processes the queue
 * @param context the context
 */
void* ipfs_bitswap_engine_wantlist_processor_start(void* ctx) {
	struct BitswapContext* context = (struct BitswapContext*)ctx;
	// the loop
	while (!context->bitswap_engine->shutting_down) {
		struct WantListQueueEntry* item = ipfs_bitswap_wantlist_queue_pop(context->localWantlist);
		if (item != NULL) {
			// if there is something on the queue process it.
			ipfs_bitswap_wantlist_process_entry(context, item);
		} else {
			// if there is nothing on the queue, wait...
			sleep(2);
		}
	}
	return NULL;
}

/***
 * A separate thread that processes the queue
 * @param context the context
 */
void* ipfs_bitswap_engine_peer_request_processor_start(void* ctx) {
	struct BitswapContext* context = (struct BitswapContext*)ctx;
	// the loop
	while (!context->bitswap_engine->shutting_down) {
		struct PeerRequest* item = ipfs_bitswap_peer_request_queue_pop(context->peerRequestQueue);
		if (item != NULL) {
			// if there is something on the queue process it.
			ipfs_bitswap_peer_request_process_entry(context, item);
		} else {
			// if there is nothing on the queue, wait...
			sleep(2);
		}
	}
	return NULL;
}

/**
 * Starts the bitswap engine that processes queue items. There
 * should only be one of these per ipfs instance.
 *
 * @param context the context
 * @returns true(1) on success, false(0) otherwise
 */
int ipfs_bitswap_engine_start(const struct BitswapContext* context) {
	context->bitswap_engine->shutting_down = 0;

	// fire off the threads
	if (pthread_create(&context->bitswap_engine->wantlist_processor_thread, NULL, ipfs_bitswap_engine_wantlist_processor_start, (void*)context)) {
		return 0;
	}
	if (pthread_create(&context->bitswap_engine->peer_request_processor_thread, NULL, ipfs_bitswap_engine_peer_request_processor_start, (void*)context)) {
		ipfs_bitswap_engine_stop(context);
		return 0;
	}
	return 1;
}

/***
 * Shut down the engine
 * @param context the context
 * @returns true(1) on success, false(0) otherwise
 */
int ipfs_bitswap_engine_stop(const struct BitswapContext* context) {
	context->bitswap_engine->shutting_down = 1;

	int error1 = pthread_join(context->bitswap_engine->wantlist_processor_thread, NULL);
	int error2 = pthread_join(context->bitswap_engine->peer_request_processor_thread, NULL);

	return !error1 && !error2;
}
