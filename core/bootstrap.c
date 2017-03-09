
#include "libp2p/peer/peer.h"
#include "ipfs/core/ipfs_node.h"
#include "ipfs/thirdparty/ipfsaddr/ipfs_addr.h"
#include "multiaddr/multiaddr.h"

/***
 * Begin to connect to the swarm
 */
void *ipfs_bootstrap_swarm(void* param) {
	struct IpfsNode* local_node = (struct IpfsNode*)param;
	// read the config file and get the bootstrap peers
	for(int i = 0; i < local_node->repo->config->peer_addresses.num_peers; i++) { // loop through the peers
		struct IPFSAddr* ipfs_addr = local_node->repo->config->peer_addresses.peers[i];
		struct MultiAddress* ma = multiaddress_new_from_string(ipfs_addr->entire_string);
		// get the id
		char* ptr;
		if ( (ptr = strstr(ipfs_addr->entire_string, "/ipfs/")) != NULL) { // look for the peer id
			ptr += 6;
			if (ptr[0] == 'Q' && ptr[1] == 'm') { // things look good
				struct Libp2pPeer* peer = libp2p_peer_new_from_data(ptr, strlen(ptr), ma);
				libp2p_peerstore_add_peer(local_node->peerstore, peer);
			}
			// TODO: attempt to connect to the peer

		} // we have a good peer ID

	}
	return (void*)1;
}
