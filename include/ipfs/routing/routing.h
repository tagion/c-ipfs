#ifndef ROUTING_H
    #define ROUTING_H

    #include "libp2p/crypto/rsa.h"

    // offlineRouting implements the IpfsRouting interface,
    // but only provides the capability to Put and Get signed dht
    // records to and from the local datastore.
    struct s_ipfs_routing {
        struct FSRepo* datastore;
        size_t ds_len;
        struct RsaPrivateKey* sk;
        int (*PutValue)      (struct s_ipfs_routing*, char*, size_t, void*, size_t);
        int (*GetValue)      (struct s_ipfs_routing*, char*, size_t, void*, size_t*);
        int (*FindProviders) (struct s_ipfs_routing*, char*, size_t, void*, size_t*);
        int (*FindPeer)      (struct s_ipfs_routing*, char*, size_t, void*, size_t*);
        int (*Provide)       (struct s_ipfs_routing*, char*);
        int (*Ping)          (struct s_ipfs_routing*, char*, size_t);
        int (*Bootstrap)     (struct s_ipfs_routing*);
   };
   typedef struct s_ipfs_routing ipfs_routing;

   // offline routing routines.
   ipfs_routing* ipfs_routing_new_offline (struct FSRepo* ds, struct RsaPrivateKey *private_key);
   int ipfs_routing_offline_put_value (ipfs_routing* offlineRouting, char *key, size_t key_size, void *val, size_t vlen);
   int ipfs_routing_offline_get_value (ipfs_routing* offlineRouting, char *key, size_t key_size, void *val, size_t *vlen);
   int ipfs_routing_offline_find_providers (ipfs_routing* offlineRouting, char *key, size_t key_size, void *ret, size_t *rlen);
   int ipfs_routing_offline_find_peer (ipfs_routing* offlineRouting, char *peer_id, size_t pid_size, void *ret, size_t *rlen);
   int ipfs_routing_offline_provide (ipfs_routing* offlineRouting, char *cid);
   int ipfs_routing_offline_ping (ipfs_routing* offlineRouting, char *peer_id, size_t pid_size);
   int ipfs_routing_offline_bootstrap (ipfs_routing* offlineRouting);
#endif // ROUTING_H
