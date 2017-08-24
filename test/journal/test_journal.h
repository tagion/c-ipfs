#include <stdlib.h>

#include "ipfs/journal/journal_entry.h"
#include "ipfs/journal/journal_message.h"

int test_journal_encode_decode() {
	int retVal = 0;
	struct JournalEntry* entry = ipfs_journal_entry_new();
	struct JournalMessage* message = ipfs_journal_message_new();
	struct JournalMessage* result_message = NULL;
	struct JournalEntry* result_entry = NULL;
	uint8_t *buffer;
	size_t buffer_size;

	// build entry
	entry->hash = malloc(1);
	entry->hash[0] = 1;
	entry->hash_size = 1;
	entry->pin = 1;
	entry->timestamp = 1;
	// build message
	message->current_epoch = 2;
	message->start_epoch = 3;
	message->end_epoch = 4;
	libp2p_utils_vector_add(message->journal_entries, entry);

	// protobuf it
	buffer_size = ipfs_journal_message_encode_size(message);
	buffer = malloc(buffer_size);
	if (!ipfs_journal_message_encode(message, buffer, buffer_size, &buffer_size))
		goto exit;

	// unprotobuf it
	if (!ipfs_journal_message_decode(buffer, buffer_size, &result_message))
		goto exit;

	// compare
	if (result_message->current_epoch != message->current_epoch)
		goto exit;
	if (result_message->start_epoch != message->start_epoch)
		goto exit;
	if (result_message->end_epoch != message->end_epoch)
		goto exit;
	if (result_message->journal_entries->total != message->journal_entries->total)
		goto exit;
	result_entry = (struct JournalEntry*) libp2p_utils_vector_get(message->journal_entries, 0);
	if (result_entry->timestamp != entry->timestamp)
		goto exit;
	if (result_entry->pin != entry->pin)
		goto exit;
	if (result_entry->hash_size != entry->hash_size)
		goto exit;
	for (int i = 0; i < result_entry->hash_size; i++) {
		if (result_entry->hash[i] != entry->hash[i])
			goto exit;
	}

	// cleanup
	retVal = 1;
	exit:
	if (buffer != NULL)
		free(buffer);
	ipfs_journal_message_free(message);
	ipfs_journal_message_free(result_message);
	// the above lines take care of these
	//ipfs_journal_entry_free(entry);
	//ipfs_journal_entry_free(result_entry);
	return retVal;
}
