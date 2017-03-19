#include <stdlib.h>

#include "ipfs/routing/routing.h"
#include "libp2p/record/message.h"
#include "libp2p/net/stream.h"

/**
 * Implements the routing interface for network clients
 */

int ipfs_routing_online_find_providers(struct s_ipfs_routing* routing, char* val1, size_t val2, void* val3, size_t* val4) {
	return 0;
}
int ipfs_routing_online_find_peer(struct s_ipfs_routing* routing, char* val1, size_t val2, void* val3, size_t* val4) {
	return 0;
}

/**
 * Notify the network that this host can provide this key
 * @param routing information about this host
 * @param val1 the key (hash) of the data
 * @returns true(1) on success, otherwise false
 */
int ipfs_routing_online_provide(struct s_ipfs_routing* routing, char* val1, size_t val2) {
	return 0;
}
int ipfs_routing_online_ping(struct s_ipfs_routing* routing, struct Libp2pMessage* message) {
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
int ipfs_routing_online_bootstrap(struct s_ipfs_routing* routing) {
	return 0;
}

/**
 * Create a new ipfs_routing struct for online clients
 * @param fs_repo the repo
 * @param private_key the local private key
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
