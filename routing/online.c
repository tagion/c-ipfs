#include <stdlib.h>

#include "ipfs/routing/routing.h"
#include "libp2p/record/message.h"
#include "libp2p/net/stream.h"

/**
 * Implements the routing interface for communicating with network clients
 */

int ipfs_routing_online_find_providers(struct IpfsRouting* routing, char* val1, size_t val2, struct Libp2pVector** multiaddresses) {
	return 0;
}

/**
 * Find a peer
 * @param routing the context
 * @param peer_id the id to look for
 * @param peer_id_size the size of the id
 * @param results the results of the search
 * @param results_size the size of results
 * @returns 0 on success, otherwise -1
 */
int ipfs_routing_online_find_peer(struct IpfsRouting* routing, const char* peer_id, size_t peer_id_size, void** results, size_t* results_size) {
	// first look to see if we have it in the peerstore
	struct Peerstore* peerstore = routing->local_node->peerstore;
	struct Libp2pPeer* peer = libp2p_peerstore_get_peer(peerstore, (unsigned char*)peer_id, peer_id_size);
	if (peer != NULL) {
		//we found it. Return it
		*results_size = libp2p_peer_protobuf_encode_size(peer);
		*results = malloc(*results_size);
		if (!libp2p_peer_protobuf_encode(peer, *results, *results_size, results_size)) {
			free(results);
			*results = NULL;
			*results_size = 0;
			return -1;
		}
		return 0;
	}
	//TODO: ask the swarm to find the peer
	return -1;
}

/**
 * Notify the network that this host can provide this key
 * @param routing information about this host
 * @param val1 the key (hash) of the data
 * @returns true(1) on success, otherwise false
 */
int ipfs_routing_online_provide(struct IpfsRouting* routing, char* val1, size_t val2) {
	return 0;
}
int ipfs_routing_online_ping(struct IpfsRouting* routing, struct Libp2pMessage* message) {
	// send the same stuff back
	size_t protobuf_size = libp2p_message_protobuf_encode_size(message);
	unsigned char protobuf[protobuf_size];

	if (!libp2p_message_protobuf_encode(message, protobuf, protobuf_size, &protobuf_size)) {
		return -1;
	}

	int retVal = routing->stream->write(routing->stream, protobuf, protobuf_size);
	if (retVal == 0)
		retVal = -1;
	return retVal;
}

/**
 * Connects to swarm
 * @param routing the routing struct
 * @returns 0 on success, otherwise error code
 */
int ipfs_routing_online_bootstrap(struct IpfsRouting* routing) {
	// for each address in our bootstrap list, add info into the peerstore
	struct Libp2pVector* bootstrap_peers = routing->local_node->repo->config->bootstrap_peers;
	for(int i = 0; i < bootstrap_peers->total; i++) {
		struct MultiAddress* address = (struct MultiAddress*)libp2p_utils_vector_get(bootstrap_peers, i);
		// attempt to get the peer ID
		const char* peer_id = multiaddress_get_peer_id(address);
		if (peer_id != NULL) {
			struct Libp2pPeer* peer = libp2p_peer_new();
			peer->id_size = strlen(peer_id);
			peer->id = malloc(peer->id_size);
			if (peer->id == NULL) { // out of memory?
				libp2p_peer_free(peer);
				return -1;
			}
			memcpy(peer->id, peer_id, peer->id_size);
			peer->addr_head = libp2p_utils_linked_list_new();
			if (peer->addr_head == NULL) { // out of memory?
				libp2p_peer_free(peer);
				return -1;
			}
			peer->addr_head->item = address;
			libp2p_peerstore_add_peer(routing->local_node->peerstore, peer);
			libp2p_peer_free(peer);
		}
	}

	return 0;
}

/**
 * Create a new ipfs_routing struct for online clients
 * @param fs_repo the repo
 * @param private_key the local private key
 * @param stream the stream to put in the struct
 * @reurns the ipfs_routing struct that handles messages
 */
ipfs_routing* ipfs_routing_new_online (struct IpfsNode* local_node, struct RsaPrivateKey *private_key, struct Stream* stream) {
    ipfs_routing *onlineRouting = malloc (sizeof(ipfs_routing));

    if (onlineRouting) {
        onlineRouting->local_node     = local_node;
        onlineRouting->sk            = private_key;
        onlineRouting->stream = stream;

        onlineRouting->PutValue      = ipfs_routing_generic_put_value;
        onlineRouting->GetValue      = ipfs_routing_generic_get_value;
        onlineRouting->FindProviders = ipfs_routing_online_find_providers;
        onlineRouting->FindPeer      = ipfs_routing_online_find_peer;
        onlineRouting->Provide       = ipfs_routing_online_provide;
        onlineRouting->Ping          = ipfs_routing_online_ping;
        onlineRouting->Bootstrap     = ipfs_routing_online_bootstrap;
    }

    return onlineRouting;
}
