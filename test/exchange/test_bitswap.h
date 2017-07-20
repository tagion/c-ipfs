#include "ipfs/exchange/bitswap/message.h"
/**
 * Test the protobufing of a BitswapMessage
 * this should be run with valgrind
 */
int test_bitswap_new_free() {
	int retVal = 0;

	// create a simple BitswapMessage
	struct BitswapMessage* message = ipfs_bitswap_message_new();
	ipfs_bitswap_message_free(message);



	retVal = 1;
	exit:
	return retVal;
}
