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
	 */
	int (*FindProviders) (struct s_ipfs_routing*, char*, size_t, void*, size_t*);
	/**
	 * Find a peer
	 */
	int (*FindPeer)      (struct s_ipfs_routing*, char*, size_t, void*, size_t*);
	int (*Provide)       (struct s_ipfs_routing*, char*);
	/**
	 * Ping this instance
	 */
	int (*Ping)          (struct s_ipfs_routing*, struct Libp2pMessage* message);
	int (*Bootstrap)     (struct s_ipfs_routing*);
};
typedef struct s_ipfs_routing ipfs_routing;

// offline routing routines.
ipfs_routing* ipfs_routing_new_offline (struct IpfsNode* local_node, struct RsaPrivateKey *private_key);
ipfs_routing* ipfs_routing_new_online (struct IpfsNode* local_node, struct RsaPrivateKey* private_key, struct Stream* stream);
int ipfs_routing_generic_put_value (ipfs_routing* offlineRouting, char *key, size_t key_size, void *val, size_t vlen);int ipfs_routing_generic_get_value (ipfs_routing* offlineRouting, char *key, size_t key_size, void **val, size_t *vlen);

