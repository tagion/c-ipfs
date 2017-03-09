#include "ipfs/routing/routing.h"

/**
 * Routing using Kademlia and DHT
 */

/**
 * Put a value in the datastore
 * @param routing the struct that contains connection information
 * @param key the key
 * @param key_size the size of the key
 * @param value the value
 * @param value_size the size of the value
 * @returns 0 on success, otherwise -1
 */
int ipfs_routing_kademlia_put_value(struct s_ipfs_routing routing, char* key, size_t key_size, void* value, size_t value_size) {

}

/**
 * Get a value from the datastore
 * @param 1 the struct that contains the connection information
 * @param 2 the key to look for
 * @param 3 the size of the key
 * @param 4 a place to store the value
 * @param 5 the size of the value
 */
int ipfs_routing_kademlia_get_value(struct s_ipfs_routing*, char*, size_t, void**, size_t*) {
	return 0;
}

/**
 * Find a provider
 */
int ipfs_routing_kademlia_find_providers(struct s_ipfs_routing*, char*, size_t, void*, size_t*) {
	return 0;
}

/**
 * Find a peer
 */
int ipfs_routing_kademlia_find_peer(struct s_ipfs_routing*, char*, size_t, void*, size_t*) {
	return 0;
}
int ipfs_routing_kademlia_provide(struct s_ipfs_routing*, char*) {
	return 0;
}

/**
 * Ping this instance
 */
int ipfs_routing_kademlia_ping(struct s_ipfs_routing* routing, struct Libp2pMessage* message) {
	return ipfs_routing_online_ping(routing, message);
}

int ipfs_routing_kademlia_bootstrap(struct s_ipfs_routing*) {
	return 0;
}

struct s_ipfs_routing* ipfs_routing_new_kademlia(struct IpfsNode* local_node, struct RsaPrivateKey* private_key, struct Stream* stream) {
	struct s_ipfs_routing* routing = (struct s_ipfs_routing*)malloc(sizeof(struct s_ipfs_routing));
	if (routing != NULL) {
		routing->local_node = local_node;
		routing->sk = private_key;
		routing->stream = stream;
		routing->PutValue = ipfs_routing_kademlia_put_value;
		routing->GetValue = ipfs_routing_kademlia_get_value;
		routing->FindProviders = ipfs_routing_kademlia_find_providers;
		routing->FindPeer = ipfs_routing_kademlia_find_peer;
		routing->Provide = ipfs_routing_kademlia_provide;
		routing->Ping = ipfs_routing_kademlia_ping;
		routing->Bootstrap = ipfs_routing_kademlia_bootstrap;
	}
	// connect to nodes and listen for connections
	struct MultiAddress* address = multiaddresss_new_from_string(local_node->repo->config->addresses->api);
	if (multiaddress_is_ip(address)) {
		int port = multiaddress_get_ip_port(address);
		int family = multiaddress_get_ip_family(address);
		start_kademlia(port, family, local_node->identity->peer_id, 10);
	}

	return routing;
}
