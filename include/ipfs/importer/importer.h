#ifndef __IPFS_IMPORTER_IMPORTER_H__
#define __IPFS_IMPORTER_IMPORTER_H__

/**
 * Creates a node based on an incoming file
 * @param file_name the file to import
 * @param node the root node (could have links to others)
 * @returns true(1) on success
 */
int ipfs_import_file(const char* fileName, struct Node** node);

#endif /* INCLUDE_IPFS_IMPORTER_IMPORTER_H_ */
