/**
 * Methods for the Bitswap exchange
 */
#include <stdlib.h>
#include <unistd.h> // for sleep()
#include "ipfs/core/ipfs_node.h"
#include "ipfs/exchange/exchange.h"
#include "ipfs/exchange/bitswap/bitswap.h"
#include "ipfs/exchange/bitswap/message.h"
#include "ipfs/exchange/bitswap/want_manager.h"

/**
 * Create a new bitswap exchange
 * @param sessionContext the context
 * @returns an allocated Exchange structure
 */
struct Exchange* ipfs_bitswap_new(struct IpfsNode* ipfs_node) {
	struct Exchange* exchange = (struct Exchange*) malloc(sizeof(struct Exchange));
	if (exchange != NULL) {
		struct BitswapContext* bitswapContext = (struct BitswapContext*) malloc(sizeof(struct BitswapContext));
		if (bitswapContext == NULL) {
			free(exchange);
			return NULL;
		}
		bitswapContext->bitswap_engine = ipfs_bitswap_engine_new();
		if (bitswapContext->bitswap_engine == NULL) {
			free(bitswapContext);
			free(exchange);
			return NULL;
		}
		bitswapContext->localWantlist = NULL;
		bitswapContext->peerRequestQueue = NULL;
		bitswapContext->ipfsNode = ipfs_node;

		exchange->exchangeContext = (void*) bitswapContext;
		exchange->IsOnline = ipfs_bitswap_is_online;
		exchange->Close = ipfs_bitswap_close;
		exchange->HasBlock = ipfs_bitswap_has_block;
		exchange->GetBlock = ipfs_bitswap_get_block;
		exchange->GetBlocks = ipfs_bitswap_get_blocks;

		// Start the threads for the network
		ipfs_bitswap_engine_start(bitswapContext);
	}
	return exchange;
}

/**
 * Clean up resources within an Exchange struct
 * @param exchange the exchange to free
 * @returns true(1)
 */
int ipfs_bitswap_free(struct Exchange* exchange) {
	if (exchange != NULL) {
		if (exchange->exchangeContext != NULL) {
			free(exchange->exchangeContext);
		}
		free(exchange);
	}
	return 1;
}

/**
 * Implements the Exchange->IsOnline method
 */
int ipfs_bitswap_is_online(struct Exchange* exchange) {
	return 1;
}

/***
 * Implements the Exchange->Close method
 */
int ipfs_bitswap_close(struct Exchange* exchange) {
	ipfs_bitswap_free(exchange);
	return 0;
}

/**
 * Implements the Exchange->HasBlock method
 * Some notes from the GO version say that this is normally called by user
 * interaction (i.e. user added a file).
 * But this does not make sense right now, as the GO code looks like it
 * adds the block to the blockstore. This still has to be sorted.
 */
int ipfs_bitswap_has_block(struct Exchange* exchange, struct Block* block) {
	//TODO: Implement this method
	// NOTE: The GO version adds the block to the blockstore. I have yet to
	// understand the flow and if this is correct for us.
	return 0;
}

/**
 * Implements the Exchange->GetBlock method
 * We're asking for this method to get the block from peers. Perhaps this should be
 * taking in a pointer to a callback, as this could take a while (or fail).
 * @param exchangeContext a BitswapContext
 * @param cid the Cid to look for
 * @param block a pointer to where to put the result
 * @returns true(1) if found, false(0) if not
 */
int ipfs_bitswap_get_block(struct Exchange* exchange, struct Cid* cid, struct Block** block) {
	struct BitswapContext* bitswapContext = (struct BitswapContext*)exchange->exchangeContext;
	if (bitswapContext != NULL) {
		// check locally first
		if (bitswapContext->ipfsNode->blockstore->Get(bitswapContext->ipfsNode->blockstore->blockstoreContext, cid, block))
			return 1;
		// now ask the network
		//NOTE: this timeout should be configurable
		int timeout = 10;
		int waitSecs = 1;
		int timeTaken = 0;
		struct WantListSession wantlist_session;
		wantlist_session.type = WANTLIST_SESSION_TYPE_LOCAL;
		wantlist_session.context = (void*)bitswapContext->ipfsNode;
		struct WantListQueueEntry* want_entry = ipfs_bitswap_want_manager_add(bitswapContext, cid, &wantlist_session);
		if (want_entry != NULL) {
			// loop waiting for it to fill
			while(1) {
				if (want_entry->block != NULL) {
					*block = ipfs_block_copy(want_entry->block);
					// error or not, we no longer need the block (decrement reference count)
					ipfs_bitswap_want_manager_remove(bitswapContext, cid);
					if (*block == NULL) {
						return 0;
					}
					return 1;
				}
				//TODO: This is a busy-loop. Find another way.
				timeTaken += waitSecs;
				if (timeTaken >= timeout) {
					// It took too long. Stop looking.
					ipfs_bitswap_want_manager_remove(bitswapContext, cid);
					break;
				}
				sleep(waitSecs);
			}
		}
	}
	return 0;
}

/**
 * Implements the Exchange->GetBlocks method
 */
int ipfs_bitswap_get_blocks(struct Exchange* exchange, struct Libp2pVector* Cids, struct Libp2pVector** blocks) {
	// TODO: Implement this method
	return 0;
}
