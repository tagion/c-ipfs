#include <stdio.h>
#include <string.h>

#include "ipfs/cid/cid.h"
#include "ipfs/merkledag/merkledag.h"
#include "ipfs/merkledag/node.h"
#include "ipfs/repo/fsrepo/fs_repo.h"
#include "ipfs/repo/init.h"
/**
 * pull objects from ipfs
 */


/***
 * Get a file by its hash, and write the data to a filestream
 * @param hash the base58 multihash of the cid
 * @param file_descriptor where to write
 * @param fs_repo the repo
 */
int ipfs_exporter_to_filestream(const unsigned char* hash, FILE* file_descriptor, const struct FSRepo* fs_repo) {
	// convert hash to cid
	struct Cid* cid = NULL;
	if ( ipfs_cid_decode_hash_from_base58(hash, strlen((char*)hash), &cid) == 0) {
		return 0;
	}

	// find block
	struct Node* read_node = NULL;
	if (ipfs_merkledag_get(cid->hash, cid->hash_length, &read_node, fs_repo) == 0) {
		ipfs_cid_free(cid);
		return 0;
	}

	// no longer need the cid
	ipfs_cid_free(cid);

	if (read_node->head_link == NULL) {
		// convert the node's data into a UnixFS data block
		struct UnixFS* unix_fs;
		ipfs_unixfs_protobuf_decode(read_node->data, read_node->data_size, &unix_fs);
		size_t bytes_written = fwrite(unix_fs->bytes, 1, unix_fs->bytes_size, file_descriptor);
		if (bytes_written != unix_fs->bytes_size) {
			fclose(file_descriptor);
			ipfs_node_free(read_node);
			ipfs_unixfs_free(unix_fs);
			return 0;
		}
		ipfs_unixfs_free(unix_fs);
	} else {
		struct NodeLink* link = read_node->head_link;
		struct Node* link_node = NULL;
		while (link != NULL) {
			if ( ipfs_merkledag_get(link->hash, link->hash_size, &link_node, fs_repo) == 0) {
				fclose(file_descriptor);
				ipfs_node_free(read_node);
				return 0;
			}
			struct UnixFS* unix_fs;
			ipfs_unixfs_protobuf_decode(link_node->data, link_node->data_size, &unix_fs);
			size_t bytes_written = fwrite(unix_fs->bytes, 1, unix_fs->bytes_size, file_descriptor);
			if (bytes_written != unix_fs->bytes_size) {
				ipfs_node_free(link_node);
				fclose(file_descriptor);
				ipfs_node_free(read_node);
				ipfs_unixfs_free(unix_fs);
				return 0;
			}
			ipfs_node_free(link_node);
			ipfs_unixfs_free(unix_fs);
			link = link->next;
		}
	}
	fclose(file_descriptor);

	if (read_node != NULL)
		ipfs_node_free(read_node);

	return 1;
}


/**
 * get a file by its hash, and write the data to a file
 * @param hash the base58 multihash of the cid
 * @param file_name the file name to write to
 * @returns true(1) on success
 */
int ipfs_exporter_to_file(const unsigned char* hash, const char* file_name, const struct FSRepo* fs_repo) {
	// process blocks
	FILE* file = fopen(file_name, "wb");
	if (file == NULL) {
		return 0;
	}
	return ipfs_exporter_to_filestream(hash, file, fs_repo);
}

/**
 * get a file by its hash, and write the data to a file
 * @param hash the base58 multihash of the cid
 * @param file_name the file name to write to
 * @returns true(1) on success
 */
int ipfs_exporter_to_console(const unsigned char* hash, const struct FSRepo* fs_repo) {
	// convert hash to cid
	struct Cid* cid = NULL;
	if ( ipfs_cid_decode_hash_from_base58(hash, strlen((char*)hash), &cid) == 0) {
		return 0;
	}

	// find block
	struct Node* read_node = NULL;
	if (ipfs_merkledag_get(cid->hash, cid->hash_length, &read_node, fs_repo) == 0) {
		ipfs_cid_free(cid);
		return 0;
	}

	// no longer need the cid
	ipfs_cid_free(cid);

	// process blocks
	struct NodeLink* link = read_node->head_link;
	printf("{Links:[");
	while (link != NULL) {
		unsigned char b58[100];
		ipfs_cid_hash_to_base58(link->hash, link->hash_size, b58, 100);
		printf("{\"Name\":\"%s\",\"Hash\":\"%s\",\"Size\":%lu}", (link->name != NULL ? link->name : ""), (char*)b58, link->t_size);
		link = link->next;
	}
	printf("],\"Data\":\"");
	for(size_t i = 0LU; i < read_node->data_size; i++) {
		printf("%02x", read_node->data[i]);
	}
	printf("\"}\n");

	if (read_node != NULL)
		ipfs_node_free(read_node);

	return 1;
}

/***
 * Called from the command line with ipfs object get [hash]. Retrieves the object pointed to by hash, and displays its block data (links and data elements)
 * @param argc number of arguments
 * @param argv arguments
 * @returns true(1) on success
 */
int ipfs_exporter_object_get(int argc, char** argv) {
	struct FSRepo* fs_repo = NULL;
	char* repo_path = NULL;

	if (!ipfs_repo_get_directory(argc, argv, &repo_path)) {
		fprintf(stderr, "Unable to open repository: %s\n", repo_path);
		return 0;
	}

	// build the struct
	int retVal = ipfs_repo_fsrepo_new(repo_path, NULL, &fs_repo);
	if (retVal == 0) {
		return 0;
	}
	// open the repo
	retVal = ipfs_repo_fsrepo_open(fs_repo);
	if (retVal == 0) {
		return 0;
	}
	// find hash
	retVal = ipfs_exporter_to_console((unsigned char*)argv[3], fs_repo);
	return retVal;
}

int ipfs_exporter_cat_node(struct Node* node, const struct FSRepo* fs_repo) {
	// process this node, then move on to the links

	// build the unixfs
	struct UnixFS* unix_fs;
	ipfs_unixfs_protobuf_decode(node->data, node->data_size, &unix_fs);
	for(size_t i = 0LU; i < unix_fs->bytes_size; i++) {
		printf("%c", unix_fs->bytes[i]);
	}
	// process links
	struct NodeLink* current = node->head_link;
	while (current != NULL) {
		// find the node
		struct Node* child_node = NULL;
		if (ipfs_merkledag_get(current->hash, current->hash_size, &child_node, fs_repo) == 0) {
			return 0;
		}
		ipfs_exporter_cat_node(child_node, fs_repo);
		ipfs_node_free(child_node);
		current = current->next;
	}

	return 1;
}

/***
 * Called from the command line with ipfs cat [hash]. Retrieves the object pointed to by hash, and displays its block data (links and data elements)
 * @param argc number of arguments
 * @param argv arguments
 * @returns true(1) on success
 */
int ipfs_exporter_object_cat(int argc, char** argv) {
	struct FSRepo* fs_repo = NULL;
	char* repo_dir = NULL;

	if (!ipfs_repo_get_directory(argc, argv, &repo_dir)) {
		fprintf(stderr, "Unable to open repo: %s\n", repo_dir);
		return 0;
	}

	// open the repo
	int retVal = ipfs_repo_fsrepo_new(repo_dir, NULL, &fs_repo);
	if (retVal == 0) {
		return 0;
	}
	retVal = ipfs_repo_fsrepo_open(fs_repo);
	if (retVal == 0) {
		return 0;
	}
	// find hash
	// convert hash to cid
	struct Cid* cid = NULL;
	if ( ipfs_cid_decode_hash_from_base58((unsigned char*)argv[2], strlen(argv[2]), &cid) == 0) {
		return 0;
	}

	// find block
	struct Node* read_node = NULL;
	if (ipfs_merkledag_get(cid->hash, cid->hash_length, &read_node, fs_repo) == 0) {
		ipfs_cid_free(cid);
		return 0;
	}
	// no longer need the cid
	ipfs_cid_free(cid);

	ipfs_exporter_cat_node(read_node, fs_repo);
	ipfs_node_free(read_node);

	return retVal;

}
