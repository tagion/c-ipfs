#include "ipfs/unixfs/unixfs.h"

int test_unixfs_encode_decode() {
	struct UnixFS* unixfs = NULL;
	int retVal;

	// a directory
	retVal = ipfs_unixfs_new(&unixfs);
	unixfs->data_type = UNIXFS_DIRECTORY;

	// serialize
	size_t buffer_size = ipfs_unixfs_protobuf_encode_size(unixfs);
	unsigned char buffer[buffer_size];
	size_t bytes_written = 0;

	retVal = ipfs_unixfs_protobuf_encode(unixfs, buffer, buffer_size, &bytes_written);
	if (retVal == 0) {
		return 0;
	}

	// unserialize
	struct UnixFS* results = NULL;
	retVal = ipfs_unixfs_protobuf_decode(buffer, bytes_written, &results);
	if (retVal == 0) {
		return 0;
	}

	// compare
	if (results->data_type != unixfs->data_type) {
		return 0;
	}

	if (results->block_size_head != unixfs->block_size_head) {
		return 0;
	}

	return 1;
}
