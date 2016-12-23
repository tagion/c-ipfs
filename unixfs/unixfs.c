/***
 * A unix-like file system over IPFS blocks
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libp2p/crypto/sha256.h"
#include "ipfs/unixfs/unixfs.h"
#include "protobuf.h"
#include "varint.h"

/**
 * The protobuf info:
 * message Data {
 *  enum DataType {
 *		Raw = 0;
 *		Directory = 1;
 *		File = 2;
 *		Metadata = 3;
 *		Symlink = 4;
 *	}
 *
 *	required DataType Type = 1;
 *	optional bytes Data = 2;
 *	optional uint64 filesize = 3;
 *	repeated uint64 blocksizes = 4;
 * }
 *
 * message Metadata {
 *	optional string MimeType = 1;
 * }
 */

/**
 * Allocate memory for a new UnixFS struct
 * @param obj the pointer to the new object
 * @returns true(1) on success
 */
int ipfs_unixfs_new(struct UnixFS** obj) {
	*obj = (struct UnixFS*)malloc(sizeof(struct UnixFS));
	if (*obj == NULL)
		return 0;
	(*obj)->bytes = 0;
	(*obj)->bytes_size = 0;
	(*obj)->data_type = UNIXFS_RAW;
	(*obj)->block_size_head = NULL;
	(*obj)->hash = NULL;
	(*obj)->hash_length = 0;
	return 1;
}

int ipfs_unixfs_free(struct UnixFS* obj) {
	if (obj != NULL) {
		free(obj);
		obj = NULL;
	}
	return 1;
}

/***
 * Write data to data section of a UnixFS stuct. NOTE: this also calculates a sha256 hash
 * @param data the data to write
 * @param data_length the length of the data
 * @param unix_fs the struct to add to
 * @returns true(1) on success
 */
int ipfs_unixfs_add_data(unsigned char* data, size_t data_length, struct UnixFS* unix_fs) {

	unix_fs->bytes_size = data_length;
	unix_fs->bytes = malloc(sizeof(unsigned char) * data_length);
	if ( unix_fs->bytes == NULL) {
		return 0;
	}
	memcpy( unix_fs->bytes, data, data_length);

	// now compute the hash
	unix_fs->hash_length = 32;
	unix_fs->hash = (unsigned char*)malloc(unix_fs->hash_length);
	if (unix_fs->hash == NULL) {
		free(unix_fs->bytes);
		return 0;
	}
	if (libp2p_crypto_hashing_sha256(data, data_length, &unix_fs->hash[0]) == 0) {
		free(unix_fs->bytes);
		free(unix_fs->hash);
		return 0;
	}

	return 1;

}


/**
 * Protobuf functions
 */

//                                            data type         bytes                    file size           block sizes
enum WireType ipfs_unixfs_message_fields[] = { WIRETYPE_VARINT, WIRETYPE_LENGTH_DELIMITED, WIRETYPE_VARINT, WIRETYPE_VARINT };

/**
 * Calculate the max size of the protobuf before encoding
 * @param obj what will be encoded
 * @returns the size of the buffer necessary to encode the object
 */
size_t ipfs_unixfs_protobuf_encode_size(const struct UnixFS* obj) {
	size_t sz = 0;
	// bytes
	sz += obj->bytes_size + 11;
	// data tupe
	sz += 2;
	// file_size
	sz += 11;
	// block sizes
	struct UnixFSBlockSizeNode* currNode = obj->block_size_head;
	while(currNode != NULL) {
		sz += 11;
		currNode = currNode->next;
	}
	return sz;
}

/***
 * Encode a UnixFS object into protobuf format
 * @param incoming the incoming object
 * @param outgoing where the bytes will be placed
 * @param max_buffer_size the size of the outgoing buffer
 * @param bytes_written how many bytes were written in the buffer
 * @returns true(1) on success
 */
int ipfs_unixfs_protobuf_encode(const struct UnixFS* incoming, unsigned char* outgoing, size_t max_buffer_size, size_t* bytes_written) {
	size_t bytes_used = 0;
	*bytes_written = 0;
	int retVal = 0;
	if (incoming != NULL) {
		// data type (required)
		retVal = protobuf_encode_varint(1, ipfs_unixfs_message_fields[0], incoming->data_type, outgoing, max_buffer_size - *bytes_written, &bytes_used);
		if (retVal == 0)
			return 0;
		*bytes_written += bytes_used;
		// bytes (optional)
		if (incoming->bytes_size > 0) {
			retVal = protobuf_encode_length_delimited(2, ipfs_unixfs_message_fields[1], (char*)incoming->bytes, incoming->bytes_size, &outgoing[*bytes_written], max_buffer_size - (*bytes_written), &bytes_used);
			if (retVal == 0)
				return 0;
			*bytes_written += bytes_used;
		}
		// file size (optional)
		if (incoming->data_type == UNIXFS_FILE && incoming->bytes_size > 0) {
			retVal = protobuf_encode_varint(3, ipfs_unixfs_message_fields[2], incoming->bytes_size, &outgoing[*bytes_written], max_buffer_size - (*bytes_written), &bytes_used);
			if (retVal == 0)
				return 0;
			*bytes_written += bytes_used;
		}
		// block sizes
		struct UnixFSBlockSizeNode* currNode = incoming->block_size_head;
		while (currNode != NULL) {
			retVal = protobuf_encode_varint(4, ipfs_unixfs_message_fields[3], currNode->block_size, &outgoing[*bytes_written], max_buffer_size - (*bytes_written), &bytes_used);
			currNode = currNode->next;
		}
	}
	return 1;
}

/***
 * Decodes a protobuf array of bytes into a UnixFS object
 * @param incoming the array of bytes
 * @param incoming_size the length of the array
 * @param outgoing the UnixFS object
 */
int ipfs_unixfs_protobuf_decode(unsigned char* incoming, size_t incoming_size, struct UnixFS** outgoing) {
	// short cut for nulls
	if (incoming_size == 0) {
		*outgoing = NULL;
		return 1;
	}

	size_t pos = 0;
	int retVal = 0;

	if (ipfs_unixfs_new(outgoing) == 0) {
		return 0;
	}
	struct UnixFS* result = *outgoing;

	while(pos < incoming_size) {
		size_t bytes_read = 0;
		int field_no;
		enum WireType field_type;
		if (protobuf_decode_field_and_type(&incoming[pos], incoming_size, &field_no, &field_type, &bytes_read) == 0) {
			return 0;
		}
		pos += bytes_read;
		switch(field_no) {
			case (1): // data type (varint)
				result->data_type = varint_decode(&incoming[pos], incoming_size - pos, &bytes_read);
				pos += bytes_read;
				break;
			case (2): // bytes (length delimited)
				retVal = protobuf_decode_length_delimited(&incoming[pos], incoming_size - pos, (char**)&(result->bytes), &(result->bytes_size), &bytes_read);
				if (retVal == 0)
					return 0;
				pos += bytes_read;
				break;
			case (3): // file size
				result->bytes_size = varint_decode(&incoming[pos], incoming_size - pos, &bytes_read);
				pos += bytes_read;
				break;
			case (4): { // block sizes (linked list from varint)
				struct UnixFSBlockSizeNode* currNode = (struct UnixFSBlockSizeNode*)malloc(sizeof(struct UnixFSBlockSizeNode));
				if (currNode == NULL)
					return 0;
				currNode->next = NULL;
				currNode->block_size = varint_decode(&incoming[pos], incoming_size - pos, &bytes_read);
				pos += bytes_read;
				if (result->block_size_head == NULL) {
					result->block_size_head = currNode;
				} else {
					struct UnixFSBlockSizeNode* last = result->block_size_head;
					while (last->next != NULL)
						last = last->next;
					last->next = currNode;
				}
				break;
			}
		}

	}

	return 1;
}
