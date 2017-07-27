#pragma once

#include <pthread.h>
//#include "ipfs/exchange/bitswap/bitswap.h" we must forward declare here, as BitswapContext has a reference to BitswapEngine

struct BitswapContext;

struct BitswapEngine {
	int shutting_down;
	pthread_t wantlist_processor_thread;
	pthread_t peer_request_processor_thread;
};

/**
 * Starts the bitswap engine that processes queue items. There
 * should only be one of these per ipfs instance.
 *
 * @param context the context
 * @returns true(1) on success, false(0) otherwise
 */
int ipfs_bitswap_engine_start(const struct BitswapContext* context);

/***
 * Shut down the engine
 *
 * @param context the context
 * @returns true(1) on success, false(0) otherwise
 */
int ipfs_bitswap_engine_stop(const struct BitswapContext* context);
