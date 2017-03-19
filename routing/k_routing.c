#include "ipfs/routing/routing.h"
#include "libp2p/routing/kademlia.h"
#include "libp2p/peer/providerstore.h"
#include "libp2p/utils/vector.h"

/**
 * Routing using Kademlia and DHT
 *
 * The go version has "supernode" which is similar to this:
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
int ipfs_routing_kademlia_put_value(struct s_ipfs_routing* routing, char* key, size_t key_size, void* value, size_t value_size) {
	return 0;
}

/**
 * Get a value from the datastore
 * @param 1 the struct that contains the connection information
 * @param 2 the key to look for
 * @param 3 the size of the key
 * @param 4 a place to store the value
 * @param 5 the size of the value
 */
int ipfs_routing_kademlia_get_value(struct s_ipfs_routing* routing, char* key, size_t key_size, void** value, size_t* value_size) {
	return 0;
}

/**
 * Find a provider
 * @param routing the context
 * @param key the key to what we're looking for
 * @param key_size the size of the key
 * @param results the results
 * @param results_size the size of the results buffer
 * @returns true(1) on success, otherwise false(0)
 */
int ipfs_routing_kademlia_find_providers(struct s_ipfs_routing* routing, char* key, size_t key_size, struct Libp2pVector** results) {
	*results = libp2p_utils_vector_new(1);
	struct Libp2pVector* vector = *results;
	// see if I can provide it
	unsigned char* peer_id = NULL;
	int peer_id_size = 0;
	if (libp2p_providerstore_get(routing->local_node->providerstore, (unsigned char*)key, key_size, &peer_id, &peer_id_size)) {
		struct Libp2pPeer* peer = libp2p_peerstore_get_peer(routing->local_node->peerstore, peer_id, peer_id_size);
		struct Libp2pLinkedList* current = peer->addr_head;
		while (current != NULL) {
			struct MultiAddress* ma = (struct MultiAddress*)current->item;
			if (multiaddress_is_ip(ma)) {
				libp2p_utils_vector_add(vector, ma);
			}
			current = current->next;
		}
	}
	//TODO: get a list of providers that are closer
	if (vector->total == 0) {
		libp2p_utils_vector_free(vector);
		vector = NULL;
		return 0;
	}
	return 1;
}

/**
 * Find a peer
 */
int ipfs_routing_kademlia_find_peer(struct s_ipfs_routing* routing, char* param1, size_t param2, void* param3, size_t* param4) {
	return 0;
}
int ipfs_routing_kademlia_provide(struct s_ipfs_routing* routing, char* key, size_t key_size) {
	//TODO: Announce to the network that I can provide this file
	// save in a cache
	// store key and address in cache. Key is the hash, peer id is the value
	libp2p_providerstore_add(routing->local_node->providerstore, (unsigned char*)key, key_size, (unsigned char*)routing->local_node->identity->peer_id, strlen(routing->local_node->identity->peer_id));

	return 1;
}

// declared here so as to have the code in 1 place
int ipfs_routing_online_ping(struct s_ipfs_routing*, struct Libp2pMessage*);
/**
 * Ping this instance
 */
int ipfs_routing_kademlia_ping(struct s_ipfs_routing* routing, struct Libp2pMessage* message) {
	return ipfs_routing_online_ping(routing, message);
}

int ipfs_routing_kademlia_bootstrap(struct s_ipfs_routing* routing) {
	return 0;
}

struct s_ipfs_routing* ipfs_routing_new_kademlia(struct IpfsNode* local_node, struct RsaPrivateKey* private_key, struct Stream* stream) {
	char* kademlia_id = NULL;
	// generate kademlia compatible id by getting last 20 chars of peer id
	if (local_node->identity->peer_id == NULL || strlen(local_node->identity->peer_id) < 20) {
		return NULL;
	}
	kademlia_id = &local_node->identity->peer_id[strlen(local_node->identity->peer_id)-20];
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
	struct MultiAddress* address = multiaddress_new_from_string(local_node->repo->config->addresses->api);
	if (multiaddress_is_ip(address)) {
		int port = multiaddress_get_ip_port(address);
		int family = multiaddress_get_ip_family(address);
		start_kademlia(port, family, kademlia_id, 10);
	}
	local_node->routing = routing;
	return routing;
}
