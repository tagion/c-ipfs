#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ipfs/namesys/routing.h"
#include "ipfs/util/time.h"
#include "mh/multihash.h"
#include "ipfs/namesys/pb.h"
#include "ipfs/namesys/namesys.h"
#include "ipfs/cid/cid.h"

char* ipfs_routing_cache_get (char *key, struct ipns_entry *ientry)
{
    int i;
    struct routingResolver *cache;
    struct stime now;

    if (key && ientry) {
        cache = ientry->cache;
        if (cache) {
            get_gmttime (&now);
            for (i = 0 ; i < cache->next ; i++) {
                if (((now.t < cache->data[i]->eol.t ||
                     (now.t == cache->data[i]->eol.t && now.ts.tv_nsec < cache->data[i]->eol.ts.tv_nsec))) &&
                    strcmp(cache->data[i]->key, key) == 0) {
                    return cache->data[i]->value;
                }
            }
        }
    }
    return NULL;
}

void ipfs_routing_cache_set (char *key, char *value, struct ipns_entry *ientry)
{
    struct cacheEntry *n;
    struct routingResolver *cache;

    if (key && value && ientry) {
        cache = ientry->cache;
        if (cache && cache->next < cache->cachesize) {
            n = malloc(sizeof (struct cacheEntry));
            if (n) {
                n->key = key;
                n->value = value;
                get_gmttime (&n->eol); // now
                n->eol.t += DefaultResolverCacheTTL; // sum TTL seconds to time seconds.
                cache->data[cache->next++] = n;
            }
        }
    }
}

// NewRoutingResolver constructs a name resolver using the IPFS Routing system
// to implement SFS-like naming on top.
// cachesize is the limit of the number of entries in the lru cache. Setting it
// to '0' will disable caching.
struct routingResolver* ipfs_namesys_new_routing_resolver (struct libp2p_routing_value_store *route, int cachesize)
{
    struct routingResolver *ret;

    if (!route) {
        fprintf(stderr, "attempt to create resolver with NULL routing system\n");
        exit (1);
    }

    ret = calloc (1, sizeof (struct routingResolver));

    if (!ret) {
        return NULL;
    }

    ret->data = calloc(cachesize, sizeof(void*));
    if (!ret) {
        free (ret);
        return NULL;
    }

    ret->cachesize = cachesize;
    return ret;
}

// ipfs_namesys_routing_resolve implements Resolver.
int ipfs_namesys_routing_resolve (char **path, char *name, struct namesys_pb *pb)
{
    return ipfs_namesys_routing_resolve_n(path, name, DefaultDepthLimit, pb);
}

// ipfs_namesys_routing_resolve_n implements Resolver.
int ipfs_namesys_routing_resolve_n (char **path, char *name, int depth, struct namesys_pb *pb)
{
    return ipfs_namesys_routing_resolve_once (path, name, depth, "/ipns/", pb);
}

// ipfs_namesys_routing_resolve_once implements resolver. Uses the IPFS
// routing system to resolve SFS-like names.
int ipfs_namesys_routing_resolve_once (char **path, char *name, int depth, char *prefix, struct namesys_pb *pb)
{
    int err, l, s, ok;
    struct MultiHash hash;
    char *h, *string, *val;
    struct libp2p_crypto_pubkey pubkey;

    if (!path || !name | !prefix) {
        return ErrInvalidParam;
    }
    // log.Debugf("RoutingResolve: '%s'", name)
    *path = ipfs_routing_cache_get (name, pb->IpnsEntry);
    if (*path) {
        return 0; // cached
    }

    if (memcmp(name, prefix, strlen(prefix)) == 0) {
        name += strlen (prefix); // trim prefix.
    }

    err = libp2p_b58_to_multihash (name, strlen(name), &hash);
    if (err) {
        // name should be a multihash. if it isn't, error out here.
        //log.Warningf("RoutingResolve: bad input hash: [%s]\n", name)
        return err;
    }

    // use the routing system to get the name.
    // /ipns/<name>
    l = strlen(prefix);
    s = (hash.size * 2) + 1;
    h = malloc(l + s); // alloc to fit prefix + hexhash + null terminator
    if (!h) {
        return ErrAllocFailed;
    }
    memcpy(h, prefix, l); // copy prefix
    if (!libp2p_multihash_hex_string(&hash, h+l, s)) { // hexstring just after prefix.
        return ErrUnknow;
    }

    err = ipfs_namesys_routing_get_value (val, h);
    if (err) {
        //log.Warning("RoutingResolve get failed.")
        return err;
    }

    //err = protobuf decode (val, pb.IpnsEntry);
    if (err) {
        return err;
    }

    // name should be a public key retrievable from ipfs
    err = ipfs_namesys_routing_getpublic_key (pubkey, &hash);
    if (err) {
        return err;
    }

    // check sig with pk
    err = libp2p_crypto_verify (ipfs_ipns_entry_data_for_sig(pb->IpnsEntry), ipfs_ipns_entry_get_signature(pb->IpnsEntry), &ok);
    if (err || !ok) {
        char buf[500];
        snprintf(buf, sizeof(buf), Err[ErrInvalidSignatureFmt], pubkey);
        l = strlen(buf) + 1;
        Err[ErrInvalidSignature] = malloc(l);
        if (!Err[ErrInvalidSignature]) {
            return ErrAllocFailed;
        }
        memcpy(Err[ErrInvalidSignature], buf, l);
        return ErrInvalidSignature;
    }
    // ok sig checks out. this is a valid name.

    // check for old style record:
    err = ipfs_namesys_pb_get_value (&string, pb->IpnsEntry);
    if (err) {
        return err;
    }
    err = libp2p_multihash_from_hex_string(string, strlen(string), &hash);
    if (err) {
        // Not a multihash, probably a new record
        err = ipfs_path_parse(path, string);
        if (err) {
            return err;
        }
    } else {
        // Its an old style multihash record
        //log.Warning("Detected old style multihash record")
        struct Cid *cid;
        err = ipfs_cid_new(0, hash.data, hash.size, CID_PROTOBUF, &cid);
        if (err) {
            return err;
        }

        err = ipfs_path_parse_from_cid (path, cid);
        if (err) {
            return err;
        }
    }

    ipfs_routing_cache_set (name, *path, pb->IpnsEntry);

    return 0;
}

int ipfs_namesys_routing_check_EOL (struct stime *st, struct namesys_pb *pb)
{
    int err;

    if (ipfs_namesys_pb_get_validity_type (pb->IpnsEntry) == IpnsEntry_EOL) {
        err = ipfs_util_time_parse_RFC3339 (st, ipfs_namesys_pb_get_validity (pb->IpnsEntry));
        if (!err) {
            return 1;
        }
    }
    return 0;
}
