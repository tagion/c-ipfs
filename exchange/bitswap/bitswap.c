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
struct Exchange* ipfs_bitswap_exchange_start(struct SessionContext* sessionContext, struct IpfsNode* ipfs_node) {
	struct Exchange* exchange = (struct Exchange*) malloc(sizeof(struct Exchange));
	if (exchange != NULL) {
		struct BitswapContext* bitswapContext = (struct BitswapContext*) malloc(sizeof(struct BitswapContext));
		if (exchange->exchangeContext == NULL) {
			free(exchange);
			return NULL;
		}
		exchange->exchangeContext = (void*) bitswapContext;
		bitswapContext->sessionContext = sessionContext;
		bitswapContext->ipfsNode = ipfs_node;
		exchange->IsOnline = ipfs_bitswap_is_online;
		exchange->Close = ipfs_bitswap_close;
		exchange->HasBlock = ipfs_bitswap_has_block;
		exchange->GetBlock = ipfs_bitswap_get_block;
		exchange->GetBlocks = ipfs_bitswap_get_blocks;
	}
	//TODO: Start the threads for the network
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
			ipfs_bitswap_close(exchange->exchangeContext);
			free(exchange->exchangeContext);
		}
		free(exchange);
	}
	return 1;
}

/**
 * Implements the Exchange->IsOnline method
 */
int ipfs_bitswap_is_online(void* exchangeContext) {
	if (exchangeContext != NULL) {
		struct BitswapContext* bitswapContext = (struct BitswapContext*)exchangeContext;
		//TODO: Is this an accurate way to determine if we're running?
		if (bitswapContext->sessionContext != NULL)
			return 1;
	}
	return 0;
}

/***
 * Implements the Exchange->Close method
 */
int ipfs_bitswap_close(void* exchangeContext) {
	//TODO: Implement this method
	// Should it close the exchange?
	return 0;
}

/**
 * Implements the Exchange->HasBlock method
 * Some notes from the GO version say that this is normally called by user
 * interaction (i.e. user added a file).
 * But this does not make sense right now, as the GO code looks like it
 * adds the block to the blockstore. This still has to be sorted.
 */
int ipfs_bitswap_has_block(void* exchangeContext, struct Block* block) {
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
int ipfs_bitswap_get_block(void* exchangeContext, struct Cid* cid, struct Block** block) {
	struct BitswapContext* bitswapContext = (struct BitswapContext*)exchangeContext;
	if (bitswapContext != NULL) {
		// check locally first
		if (bitswapContext->ipfsNode->blockstore->Get(bitswapContext->ipfsNode->blockstore->blockstoreContext, cid, block))
			return 1;
		// now ask the network
		//NOTE: this timeout should be configurable
		int timeout = 10;
		int waitSecs = 1;
		int timeTaken = 0;
		struct WantListQueueEntry* want_entry = ipfs_bitswap_want_manager_add(bitswapContext, cid);
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
int ipfs_bitswap_get_blocks(void* exchangeContext, struct Libp2pVector* Cids, struct Libp2pVector** blocks) {
	// TODO: Implement this method
	return 0;
}
