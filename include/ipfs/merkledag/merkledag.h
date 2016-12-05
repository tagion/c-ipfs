#ifndef __IPFS_MERKLEDAG_H__
#define __IPFS_MERKLEDAG_H__

#include "ipfs/node/node.h"
#include "ipfs/repo/fsrepo/fs_repo.h"

/***
 * Adds a node to the dagService and blockService
 * @param node the node to add
 * @param cid the resultant cid that was added
 * @returns true(1) on success
 */
int ipfs_merkledag_add(struct Node* node, struct FSRepo* fs_repo);


#endif
