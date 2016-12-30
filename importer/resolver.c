#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "libp2p/crypto/encoding/base58.h"
#include "ipfs/merkledag/node.h"
#include "ipfs/merkledag/merkledag.h"
#include "ipfs/repo/fsrepo/fs_repo.h"

/**
 * return the next chunk of a path
 * @param path the path
 * @param next_part a pointer to a string NOTE: don't forget to free
 * @returns true(1) on success, false(0) on error, or no more parts
 */
int ipfs_resolver_next_path(const char* path, char** next_part) {
	for (int i = 0; i < strlen(path); i++) {
		if (path[i] != '/') { // we have the next section
			char* pos = strchr(&path[i+1], '/');
			if (pos == NULL) {
				*next_part = (char*)malloc(strlen(path) + 1);
				strcpy(*next_part, path);
			} else {
				*next_part = (char*)malloc(pos - &path[i] + 1);
				strncpy(*next_part, &path[i], pos-&path[i]);
				(*next_part)[pos-&path[i]] = 0;
			}
			return 1;
		}
	}
	return 0;
}

/**
 * Remove preceding slash and "/ipfs/" or "/ipns/"
 * @param path
 */
const char* ipfs_resolver_remove_path_prefix(const char* path) {
	int pos = 0;
	int first_non_slash = -1;
	while(&path[pos] != NULL) {
		if (path[pos] == '/') {
			pos++;
			continue;
		} else {
			if (first_non_slash == -1)
				first_non_slash = pos;
			if (pos == first_non_slash && (strncmp(&path[pos], "ipfs", 4) == 0 || strncmp(&path[pos], "ipns", 4) == 0) ) {
				// ipfs or ipns should be up front. Otherwise, it could be part of the path
				pos += 4;
			} else {
				return &path[pos];
			}
		}
	}
	return NULL;
}

/**
 * Interogate the path and the current node, looking
 * for the desired node.
 * @param path the current path
 * @param from the current node (or NULL if it is the first call)
 * @returns what we are looking for, or NULL if it wasn't found
 */
struct Node* ipfs_resolver_get(const char* path, struct Node* from, const struct FSRepo* fs_repo) {
	// remove unnecessary stuff
	if (from == NULL)
		path = ipfs_resolver_remove_path_prefix(path);
	// grab the portion of the path to work with
	char* path_section;
	if (ipfs_resolver_next_path(path, &path_section) == 0)
		return NULL;
	struct Node* current_node = NULL;
	if (from == NULL) {
		// this is the first time around. Grab the root node
		if (path_section[0] == 'Q' && path_section[1] == 'm') {
			// we have a hash. Convert to a real hash, and find the node
			size_t hash_length = libp2p_crypto_encoding_base58_decode_size(strlen(path_section));
			unsigned char hash[hash_length];
			unsigned char* ptr = &hash[0];
			if (libp2p_crypto_encoding_base58_decode((unsigned char*)path_section, strlen(path_section), &ptr, &hash_length) == 0) {
				free(path_section);
				return NULL;
			}
			if (ipfs_merkledag_get_by_multihash(hash, hash_length, &current_node, fs_repo) == 0) {
				free(path_section);
				return NULL;
			}
			// we have the root node, now see if we want this or something further down
			int pos = strlen(path_section);
			if (pos == strlen(path)) {
				return current_node;
			} else {
				// look on...
				return ipfs_resolver_get(&path[pos+1], current_node, fs_repo); // the +1 is the slash
			}
		} else {
			// we don't have a current node, and we don't have a hash. Something is wrong
			free(path_section);
			return NULL;
		}
	} else {
		// we were passed a node. If it is a directory, see if what we're looking for is in it
		if (ipfs_node_is_directory(from)) {
			struct NodeLink* curr_link = from->head_link;
			while (curr_link != NULL) {
				// if it matches the name, we found what we're looking for.
				// If so, load up the node by its hash
				if (strcmp(curr_link->name, path_section) == 0) {
					if (ipfs_merkledag_get(curr_link->hash, curr_link->hash_size, &current_node, fs_repo) == 0) {
						free(path_section);
						return NULL;
					}
					if (strlen(path_section) == strlen(path)) {
						// we are at the end of our search
						ipfs_node_free(from);
						return current_node;
					} else {
						char* next_path_section;
						ipfs_resolver_next_path(&path[strlen(path_section)], &next_path_section);
						free(path_section);
						// if we're at the end of the path, return the node
						// continue looking for the next part of the path
						ipfs_node_free(from);
						return ipfs_resolver_get(next_path_section, current_node, fs_repo);
					}
				}
				curr_link = curr_link->next;
			}
		} else {
			// we're asking for a file from an object that is not a directory. Bail.
			free(path_section);
			return NULL;
		}
	}
	// it should never get here
	free(path_section);
	if (from != NULL)
		ipfs_node_free(from);
	return NULL;
}
