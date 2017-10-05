#include <stdlib.h>
#include <pthread.h>
#include <ipfs/routing/routing.h>
#include <ipfs/util/errs.h>
#include "libp2p/crypto/rsa.h"
#include "libp2p/utils/logger.h"
#include "libp2p/record/record.h"
#include "ipfs/core/http_request.h"
#include "ipfs/datastore/ds_helper.h"
#include "ipfs/merkledag/merkledag.h"
#include "ipfs/routing/routing.h"
#include "ipfs/importer/resolver.h"

int ipfs_routing_generic_put_value (ipfs_routing* offlineRouting, const unsigned char *key, size_t key_size, const void *val, size_t vlen)
{
    int err;
    char *record, *nkey;
    size_t len, nkey_len;

    err = libp2p_record_make_put_record (&record, &len, offlineRouting->sk, (const char*)key, val, vlen, 0);

    if (err) {
        return err;
    }

    nkey = malloc(key_size * 2); // FIXME: size of encoded key
    if (!nkey) {
        free (record);
        return -1;
    }

    if (!ipfs_datastore_helper_ds_key_from_binary(key, key_size, (unsigned char*)nkey, key_size+1, &nkey_len)) {
        free (nkey);
        free (record);
        return -1;
    }

    // TODO: Save to db as offline storage.
    free (record);
    return 0; // success.
}

/***
 * Get a value from the merkledag
 * @param routing the context
 * @param key the key
 * @param key_size the size of the key
 * @param val where to put the results
 * @param vlen the size of the results
 * @returns true(1) on success, false(0) otherwise
 */
int ipfs_routing_generic_get_value (ipfs_routing* routing, const unsigned char *key, size_t key_size, void **val, size_t *vlen)
{
	int retVal = 0;

	if (routing->local_node->mode == MODE_API_AVAILABLE) {
		unsigned char buffer[256];
		if (!ipfs_cid_hash_to_base58(key, key_size, &buffer[0], 256)) {
			libp2p_logger_error("offline", "Unable to convert hash to its Base58 representation.\n");
			return 0;
		}

		libp2p_logger_debug("offline", "Attempting to ask API for dht get %s.\n", buffer);

		char* response;
		struct HttpRequest* req = ipfs_core_http_request_new();
		req->command = "dht";
		req->sub_command = "get";
		req->arguments = libp2p_utils_vector_new(1);
		libp2p_utils_vector_add(req->arguments, buffer);
		size_t response_size = 0;
		if (!ipfs_core_http_request_get(routing->local_node, req, &response, &response_size)) {
			libp2p_logger_error("offline", "Unable to call API for dht get.\n");
			ipfs_core_http_request_free(req);
			return 0;
		}
		ipfs_core_http_request_free(req);
		*vlen = response_size;
		if (*vlen > 0) {
			*val = malloc(*vlen + 1);
			uint8_t* ptr = (uint8_t*)*val;
			if (ptr == NULL) {
				return 0;
			}
			memcpy(ptr, response, *vlen);
			ptr[*vlen] = 0;
			retVal = 1;
		}
	} else {
	    struct HashtableNode* node = NULL;
	    *val = NULL;

	    if (!ipfs_merkledag_get(key, key_size, &node, routing->local_node->repo)) {
			goto exit;
		}

	    // protobuf the node
	    int protobuf_size = ipfs_hashtable_node_protobuf_encode_size(node);
	    *val = malloc(protobuf_size);
	    if (*val == NULL)
	    	goto exit;

	    if (ipfs_hashtable_node_protobuf_encode(node, *val, protobuf_size, vlen) == 0) {
	    		goto exit;
	    }

	    retVal = 1;
	    exit:
		if (node != NULL)
			ipfs_hashtable_node_free(node);
		if (retVal != 1 && *val != NULL) {
			free(*val);
			*val = NULL;
		}
	}
    return retVal;
}

int ipfs_routing_offline_find_providers (ipfs_routing* offlineRouting, const unsigned char *key, size_t key_size, struct Libp2pVector** peers)
{
    return ErrOffline;
}

int ipfs_routing_offline_find_peer (ipfs_routing* offlineRouting, const unsigned char *peer_id, size_t pid_size, struct Libp2pPeer **result)
{
    return ErrOffline;
}

/**
 * Attempt to publish that this node can provide a value
 * @param offlineRouting the context
 * @param incoming_hash the hash (in binary form)
 * @param incoming_hash_size the length of the hash array
 * @returns true(1) on success, false(0) otherwise
 */
int ipfs_routing_offline_provide (ipfs_routing* offlineRouting, const unsigned char *incoming_hash, size_t incoming_hash_size)
{
	if (offlineRouting->local_node->mode == MODE_API_AVAILABLE) {
		//TODO: publish this through the api
		unsigned char buffer[256];
		if (!ipfs_cid_hash_to_base58(incoming_hash, incoming_hash_size, &buffer[0], 256)) {
			libp2p_logger_error("offline", "Unable to convert hash to its Base58 representation.\n");
			return 0;
		}

		char* response;
		struct HttpRequest* request = ipfs_core_http_request_new();
		request->command = "dht";
		request->sub_command = "provide";
		libp2p_utils_vector_add(request->arguments, buffer);
		size_t response_size = 0;
		if (!ipfs_core_http_request_get(offlineRouting->local_node, request, &response, &response_size)) {
			libp2p_logger_error("offline", "Unable to call API for dht publish.\n");
			ipfs_core_http_request_free(request);
			return 0;
		}
		ipfs_core_http_request_free(request);
		fwrite(response, 1, response_size, stdout);
		return 1;
	} else {
		libp2p_logger_debug("offline", "Unable to announce that I can provide the hash, as API not available.\n");
	}

    return 0;
}

int ipfs_routing_offline_ping (ipfs_routing* offlineRouting, struct Libp2pPeer* peer)
{
    return ErrOffline;
}

/**
 * For offline, this does nothing
 * @param offlineRouting the interface
 * @returns 0
 */
int ipfs_routing_offline_bootstrap (ipfs_routing* offlineRouting)
{
    return 0;
}

struct IpfsRouting* ipfs_routing_new_offline (struct IpfsNode* local_node, struct RsaPrivateKey *private_key)
{
    struct IpfsRouting *offlineRouting = malloc (sizeof(struct IpfsRouting));

    if (offlineRouting) {
        offlineRouting->local_node     = local_node;
        offlineRouting->sk            = private_key;

        offlineRouting->PutValue      = ipfs_routing_generic_put_value;
        offlineRouting->GetValue      = ipfs_routing_generic_get_value;
        offlineRouting->FindProviders = ipfs_routing_offline_find_providers;
        offlineRouting->FindPeer      = ipfs_routing_offline_find_peer;
        offlineRouting->Provide       = ipfs_routing_offline_provide;
        offlineRouting->Ping          = ipfs_routing_offline_ping;
        offlineRouting->Bootstrap     = ipfs_routing_offline_bootstrap;
    }

    return offlineRouting;
}

int ipfs_routing_offline_free(ipfs_routing* incoming) {
	free(incoming);
	return 1;
}
