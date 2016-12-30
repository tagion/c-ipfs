#pragma once

#include "ipfs/merkledag/node.h"

/**
 * Interogate the path and the current node, looking
 * for the desired node.
 * @param path the current path
 * @param from the current node (or NULL if it is the first call)
 * @returns what we are looking for, or NULL if it wasn't found
 */
struct Node* ipfs_resolver_get(const char* path, struct Node* from, const struct FSRepo* fs_repo);
