#include "ipfs/exchange/bitswap/want_manager.h"
#include "ipfs/exchange/bitswap/wantlist.h"

/***
 * Add a Cid to the local wantlist
 * @param context the context
 * @param cid the Cid
 * @returns true(1) on success, false(0) otherwise
 */
int ipfs_bitswap_want_manager_add(const struct BitswapContext* context, const struct Cid* cid) {
	// first see if it is already here
	// increment the reference count
	return 0;
}

/***
 * Checks to see if the requested block has been received
 * @param context the context
 * @param cid the Cid
 * @returns true(1) if it has been received, false(0) otherwise
 */
int ipfs_bitswap_want_manager_received(const struct BitswapContext* context, const struct Cid* cid) {
	// find the entry
	// check the status
	return 0;
}

/***
 * retrieve a block from the WantManager. NOTE: a call to want_manager_received should be done first
 * @param context the context
 * @param cid the Cid to get
 * @param block a pointer to the block that will be allocated
 * @returns true(1) on success, false(0) otherwise
 */
int ipfs_bitswap_want_manager_get_block(const struct BitswapContext* context, const struct Cid* cid, struct Block** block) {
	// find the entry
	// return a copy of the block
	return 0;
}

/***
 * We no longer are requesting this block, so remove it from the queue
 * NOTE: This is reference counted, as another process may have asked for it.
 * @param context the context
 * @param cid the Cid
 * @returns true(1) on success, false(0) otherwise.
 */
int ipfs_bitswap_want_manager_remove(const struct BitswapContext* context, const struct Cid* cid) {
	// decrement the reference count
	// if it is zero, remove the entry (lock first)
	return 0;
}
