#pragma once

#include "ipfs/core/ipfs_node.h"

/**
 * Pull bytes from the hashtable
 */

/**
 * get a file by its hash, and write the data to a file
 * @param hash the base58 multihash of the cid
 * @param file_name the file name to write to
 * @returns true(1) on success
 */
int ipfs_exporter_to_file(const unsigned char* hash, const char* file_name, const struct FSRepo* fs_repo);

/***
 * Retrieve a protobuf'd Node from the router
 * @param local_node the context
 * @param hash the hash to retrieve
 * @param hash_size the length of the hash
 * @param result a place to store the Node
 * @returns true(1) on success, otherwise false(0)
 */
int ipfs_exporter_get_node(struct IpfsNode* local_node, const unsigned char* hash, const size_t hash_size, struct HashtableNode** result);

int ipfs_exporter_object_get(int argc, char** argv);

/***
 * Called from the command line with ipfs cat [hash]. Retrieves the object pointed to by hash, and displays its block data (links and data elements)
 * @param argc number of arguments
 * @param argv arguments
 * @returns true(1) on success
 */
int ipfs_exporter_object_cat(int argc, char** argv);
