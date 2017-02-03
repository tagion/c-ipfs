#include <stdlib.h>
#include <ipfs/routing/routing.h>
#include <ipfs/util/errs.h>
#include "libp2p/crypto/rsa.h"
#include "libp2p/record/record.h"
#include "ipfs/datastore/ds_helper.h"
#include "ipfs/merkledag/merkledag.h"

ipfs_routing* ipfs_routing_new_offline (struct FSRepo* ds, struct RsaPrivateKey *private_key)
{
    ipfs_routing *offlineRouting = malloc (sizeof(ipfs_routing));

    if (offlineRouting) {
        offlineRouting->datastore     = ds;
        offlineRouting->sk            = private_key;

        offlineRouting->PutValue      = ipfs_routing_offline_put_value;
        offlineRouting->GetValue      = ipfs_routing_offline_get_value;
        offlineRouting->FindProviders = ipfs_routing_offline_find_providers;
        offlineRouting->FindPeer      = ipfs_routing_offline_find_peer;
        offlineRouting->Provide       = ipfs_routing_offline_provide;
        offlineRouting->Ping          = ipfs_routing_offline_ping;
        offlineRouting->Bootstrap     = ipfs_routing_offline_bootstrap;
    }

    return offlineRouting;
}

int ipfs_routing_offline_put_value (ipfs_routing* offlineRouting, char *key, size_t key_size, void *val, size_t vlen)
{
    int err;
    char *record, *nkey;
    size_t len, nkey_len;

    err = libp2p_record_make_put_record (&record, &len, offlineRouting->sk, key, val, vlen, 0);

    if (err) {
        return err;
    }

    nkey = malloc(key_size * 2); // FIXME: size of encoded key
    if (!nkey) {
        free (record);
        return -1;
    }

    if (!ipfs_datastore_helper_ds_key_from_binary((unsigned char*)key, key_size, (unsigned char*)nkey, key_size+1, &nkey_len)) {
        free (nkey);
        free (record);
        return -1;
    }

    // TODO: Save to db as offline storage.
    free (record);
    return 0; // success.
}

int ipfs_routing_offline_get_value (ipfs_routing* offlineRouting, char *key, size_t key_size, void *val, size_t *vlen)
{
    // TODO: Read from db, validate and decode before return.
    return -1;
}

int ipfs_routing_offline_find_providers (ipfs_routing* offlineRouting, char *key, size_t key_size, void *ret, size_t *rlen)
{
    return ErrOffline;
}

int ipfs_routing_offline_find_peer (ipfs_routing* offlineRouting, char *peer_id, size_t pid_size, void *ret, size_t *rlen)
{
    return ErrOffline;
}

int ipfs_routing_offline_provide (ipfs_routing* offlineRouting, char *cid)
{
    return ErrOffline;
}

int ipfs_routing_offline_ping (ipfs_routing* offlineRouting, char *peer_id, size_t pid_size)
{
    return ErrOffline;
}

int ipfs_routing_offline_bootstrap (ipfs_routing* offlineRouting)
{
    return ErrOffline;
}
