struct BitswapContext {
	struct SessionContext* sessionContext;
};

int ipfs_bitswap_is_online(void* exchangeContext);

int ipfs_bitswap_close(void* exchangeContext);

int ipfs_bitswap_has_block(void* exchangeContext, struct Block* block);

int ipfs_bitswap_get_block(void* exchangeContext, struct Cid* cid, struct Block** block);

int ipfs_bitswap_get_blocks(void* exchangeContext, struct Libp2pVector* Cids, struct Libp2pVector** blocks);
