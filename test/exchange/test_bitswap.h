#include "ipfs/exchange/bitswap/message.h"
#include "libp2p/utils/vector.h"

uint8_t* generate_bytes(size_t size) {
	uint8_t* buffer = (uint8_t*) malloc(size);
	for(size_t i = 0; i < size; i++) {
		buffer[i] = i % 255;
	}
	return buffer;
}

int compare_generated_bytes(uint8_t* buffer, size_t size) {
	for(size_t i = 0; i < size; i++) {
		if (buffer[i] != (i % 255)) {
			fprintf(stderr, "compare_generated_bytes: Mismatch in position %lu", i);
			return 0;
		}
	}
	return 1;
}

/**
 * Test the protobufing of a BitswapMessage
 * this should be run with valgrind
 */
int test_bitswap_new_free() {
	int retVal = 0;
	struct BitswapMessage* message = NULL;
	struct WantlistEntry* wantlist_entry = NULL;
	struct Block* block = NULL;

	// Test 1, create a simple BitswapMessage
	message = ipfs_bitswap_message_new();
	ipfs_bitswap_message_free(message);
	message = NULL;

	// Test 2, okay, that worked, now make one more complicated
	message = ipfs_bitswap_message_new();
	message->wantlist = ipfs_bitswap_wantlist_new();
	ipfs_bitswap_message_free(message);
	message = NULL;

	// Test 3, now add some more
	message = ipfs_bitswap_message_new();
	// wantlist
	message->wantlist = ipfs_bitswap_wantlist_new();
	wantlist_entry = ipfs_bitswap_wantlist_entry_new();
	wantlist_entry->priority = 24;
	message->wantlist->entries = libp2p_utils_vector_new(1);
	libp2p_utils_vector_add(message->wantlist->entries, wantlist_entry);
	wantlist_entry = NULL;
	wantlist_entry = libp2p_utils_vector_get(message->wantlist->entries, 0);
	if (wantlist_entry == NULL) {
		fprintf(stderr, "Vector didn't have entry\n");
		goto exit;
	}
	if (wantlist_entry->priority != 24) {
		fprintf(stderr, "Unable to retrieve item from vector after an external free\n");
		goto exit;
	}
	// payload
	message->payload = libp2p_utils_vector_new(1);
	block = ipfs_blocks_block_new();
	block->data_length = 25;
	libp2p_utils_vector_add(message->payload, block);
	block = libp2p_utils_vector_get(message->payload, 0);
	if (block == NULL) {
		fprintf(stderr, "Vector didn't have payload entry\n");
		goto exit;
	}
	if (block->data_length != 25) {
		fprintf(stderr, "Unable to retrieve block->data_length\n");
	}

	retVal = 1;
	exit:

	if (message != NULL)
	{
		ipfs_bitswap_message_free(message);
	}

	return retVal;
}

int test_bitswap_protobuf() {
	int retVal = 0;

	// create a complicated BitswapMessage
	// first the basics...
	struct BitswapMessage* message = ipfs_bitswap_message_new();
	// add a WantList
	struct BitswapWantlist* want_list = ipfs_bitswap_wantlist_new();
	message->wantlist = want_list;
	// add something to the WantList
	message->wantlist->entries = libp2p_utils_vector_new(1);
	struct WantlistEntry* wantlist_entry = ipfs_bitswap_wantlist_entry_new();
	wantlist_entry->block_size = 1;
	wantlist_entry->priority = 100;
	wantlist_entry->block = generate_bytes(100);
	wantlist_entry->block_size = 100;
	libp2p_utils_vector_add(message->wantlist->entries, wantlist_entry);
	message->wantlist->full = 1;

	retVal = 1;
	exit:
	return retVal;
}
