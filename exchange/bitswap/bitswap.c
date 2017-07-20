/**
 * Methods for the Bitswap exchange
 */
#include <stdlib.h>
#include "ipfs/exchange/exchange.h"
#include "ipfs/exchange/bitswap/bitswap.h"
#include "ipfs/exchange/bitswap/message.h"

/**
 * Create a new bitswap exchange
 * @param sessionContext the context
 * @returns an allocated Exchange structure
 */
struct Exchange* ipfs_bitswap_new(struct SessionContext* sessionContext) {
	struct Exchange* exchange = (struct Exchange*) malloc(sizeof(struct Exchange));
	if (exchange != NULL) {
		struct BitswapContext* bitswapContext = (struct BitswapContext*) malloc(sizeof(struct BitswapContext));
		if (exchange->exchangeContext == NULL) {
			free(exchange);
			return NULL;
		}
		exchange->exchangeContext = (void*) bitswapContext;
		bitswapContext->sessionContext = sessionContext;
		//TODO: fill in the exchangeContext
		exchange->IsOnline = ipfs_bitswap_is_online;
		exchange->Close = ipfs_bitswap_close;
		exchange->HasBlock = ipfs_bitswap_has_block;
		exchange->GetBlock = ipfs_bitswap_get_block;
		exchange->GetBlocks = ipfs_bitswap_get_blocks;
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
 */
int ipfs_bitswap_has_block(void* exchangeContext, struct Block* block) {
	//TODO: Implement this method
	// NOTE: The GO version adds the block to the blockstore. I have yet to
	// understand the flow and if this is correct for us.
	return 0;
}

/**
 * Implements the Exchange->GetBlock method
 */
int ipfs_bitswap_get_block(void* exchangeContext, struct Cid* cid, struct Block** block) {
	// TODO: Implement this method
	return 0;
}

/**
 * Implements the Exchange->GetBlocks method
 */
int ipfs_bitswap_get_blocks(void* exchangeContext, struct Libp2pVector* Cids, struct Libp2pVector** blocks) {
	// TODO: Implement this method
	return 0;
}

/***
 * Receive a BitswapMessage from a peer.
 * @param exchangeContext the context
 * @param peer_id the origin
 * @param peer_id_size the size of the peer_id
 * @param message the message
 * @returns true(1) on success, otherwise false(0)
 */
int ipfs_bitswap_receive_message(void* exchangeContext, unsigned char* peer_id, int peer_id_size, struct BitswapMessage* message) {

	return 0;
}
