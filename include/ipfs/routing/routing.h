#pragma once

#include "libp2p/crypto/rsa.h"
#include "libp2p/record/message.h"
#include "ipfs/core/ipfs_node.h"

// offlineRouting implements the IpfsRouting interface,
// but only provides the capability to Put and Get signed dht
// records to and from the local datastore.
struct s_ipfs_routing {
	struct IpfsNode* local_node;
	size_t ds_len;
	struct RsaPrivateKey* sk;
	struct Stream* stream;

	/**
	 * Put a value in the datastore
	 * @param 1 the struct that contains connection information
	 * @param 2 the key
	 * @param 3 the size of the key
	 * @param 4 the value
	 * @param 5 the size of the value
	 * @returns 0 on success, otherwise -1
	 */
	int (*PutValue)      (struct s_ipfs_routing*, char*, size_t, void*, size_t);
	/**
	 * Get a value from the datastore
	 * @param 1 the struct that contains the connection information
	 * @param 2 the key to look for
	 * @param 3 the size of the key
	 * @param 4 a place to store the value
	 * @param 5 the size of the value
	 */
	int (*GetValue)      (struct s_ipfs_routing*, char*, size_t, void**, size_t*);
	/**
	 * Find a provider
	 * @param 1 the context
	 * @param 2 the information that is being looked for
	 * @param 3 the size of param 2
	 * @param 4 the information found
	 * @param 5 the size of the information found
	 */
	int (*FindProviders) (struct s_ipfs_routing*, char*, size_t, void*, size_t*);
	/**
	 * Find a peer
	 * @param 1 the context
	 * @param 2 the peer to look for
	 * @param 3 the size of the peer char array
	 * @param 4 the results
	 * @param 5 the size of the results
	 */
	int (*FindPeer)      (struct s_ipfs_routing*, char*, size_t, void*, size_t*);
	/**
	 * Announce to the network that this host can provide this key
	 * @param 1 the context
	 * @param 2 the key
	 * @param 3 the key size
	 * @returns true(1) on success, otherwise false(0)
	 */
	int (*Provide)       (struct s_ipfs_routing*, char*, size_t);
	/**
	 * Ping
	 * @param routing the context
	 * @param message the message
	 * @returns true(1) on success, otherwise false(0)
	 */
	int (*Ping)          (struct s_ipfs_routing*, struct Libp2pMessage*);
	/**
	 * Get everything going
	 * @param routing the context
	 * @returns true(1) on success, otherwise false(0)
	 */
	int (*Bootstrap)     (struct s_ipfs_routing*);
};
typedef struct s_ipfs_routing ipfs_routing;

// offline routing routines.
ipfs_routing* ipfs_routing_new_offline (struct IpfsNode* local_node, struct RsaPrivateKey *private_key);
// online using secio, should probably be deprecated
ipfs_routing* ipfs_routing_new_online (struct IpfsNode* local_node, struct RsaPrivateKey* private_key, struct Stream* stream);
// online using DHT/kademlia, the recommended router
ipfs_routing* ipfs_routing_new_kademlia(struct IpfsNode* local_node, struct RsaPrivateKey* private_key, struct Stream* stream);
// generic routines
int ipfs_routing_generic_put_value (ipfs_routing* offlineRouting, char *key, size_t key_size, void *val, size_t vlen);
int ipfs_routing_generic_get_value (ipfs_routing* offlineRouting, char *key, size_t key_size, void **val, size_t *vlen);

// supernode
int ipfs_routing_supernode_parse_provider(const unsigned char* in, size_t in_size, struct Libp2pLinkedList** multiaddresses);
