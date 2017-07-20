/***
 * A protobuf-able Bitswap Message
 */
#include <stdint.h>
#include <stddef.h>
#include "libp2p/utils/vector.h"

struct WantlistEntry {
	// optional string block = 1, the block cid (cidV0 in bitswap 1.0.0, cidV1 in bitswap 1.1.0
	unsigned char* block;
	size_t block_size;
	// optional int32 priority = 2, the priority (normalized). default to 1
	uint32_t priority;
	// optional bool cancel = 3, whether this revokes an entry
	uint8_t cancel;
};

struct BitswapWantlist {
	// repeated WantlistEntry entries = 1, a list of wantlist entries
	struct Libp2pVector* entries;
	// optional bool full = 2, whether this is the full wantlist. default to false
	uint8_t full;
};

struct BitswapBlock {
	// optional bytes prefix = 1, // CID prefix (cid version, multicodec, and multihash prefix (type + length))
	uint8_t* prefix;
	size_t prefix_size;
	// optional bytes data = 2
	uint8_t* bytes;
	size_t bytes_size;
};

struct BitswapMessage {
	// optional Wantlist wantlist = 1
	struct BitswapWantlist* wantlist;
	// repeated bytes blocks = 2, used to send Blocks in bitswap 1.0.0
	struct Libp2pVector* blocks;
	// repeated Block payload = 3, used to send Blocks in bitswap 1.1.0
	struct Libp2pVector* payload;

};
