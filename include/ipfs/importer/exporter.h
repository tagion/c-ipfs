#pragma once

/**
 * get a file by its hash, and write the data to a file
 * @param hash the base58 multihash of the cid
 * @param file_name the file name to write to
 * @returns true(1) on success
 */
int ipfs_exporter_to_file(const unsigned char* hash, const char* file_name, const struct FSRepo* fs_repo);
