/***
 * This implements the BitswapNetwork. Members of this network can fill requests and
 * smartly handle queues of local and remote requests.
 *
 * For a somewhat accurate diagram of how this may work, @see https://github.com/ipfs/js-ipfs-bitswap
 */

#include "ipfs/exchange/bitswap/network.h"

/***
 * The main loop
 */

/**
 * We received a BitswapMessage from the network
 */
/*
ipfs_bitswap_network_receive_message(struct BitswapContext* context) {

}
*/

/**
 * We want to pop something off the queue
 */

/****
 * send a message to a particular peer
 * @param context the BitswapContext
 * @param peer the peer that is the recipient
 * @param message the message to send
 */
int ipfs_bitswap_network_send_message(const struct BitswapContext* context, const struct Libp2pPeer* peer, const struct BitswapMessage* message) {
	return 0;
}

