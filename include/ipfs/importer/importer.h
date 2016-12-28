#ifndef __IPFS_IMPORTER_IMPORTER_H__
#define __IPFS_IMPORTER_IMPORTER_H__

#include "ipfs/merkledag/node.h"
#include "ipfs/repo/fsrepo/fs_repo.h"

/**
 * Creates a node based on an incoming file
 * @param file_name the file to import
 * @param node the root node (could have links to others)
 * @returns true(1) on success
 */
int ipfs_import_file(const char* fileName, struct Node** node, struct FSRepo* fs_repo);

/**
 * called from the command line
 * @param argc the number of arguments
 * @param argv the arguments
 */
int ipfs_import_files(int argc, char** argv);

#endif /* INCLUDE_IPFS_IMPORTER_IMPORTER_H_ */
