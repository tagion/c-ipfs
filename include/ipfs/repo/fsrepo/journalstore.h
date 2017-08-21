#pragma once
/**
 * Piggyback on the datastore to access the journal entries
 */

#include <stdint.h>
#include <stdlib.h>

#include "libp2p/db/datastore.h"

struct JournalRecord {
	unsigned long long timestamp;
	int pin;
	uint8_t *hash;
	size_t hash_size;
};

/**
 * Open a cursor to the journalstore table
 */
int repo_journalstore_cursor_open(struct Datastore* datastore, void** cursor);

/**
 * Read a record from the cursor
 */
int repo_journalstore_cursor_get(struct Datastore* datastore, void* cursor, enum DatastoreCursorOp op, struct JournalRecord** record);

/**
 * Close the cursor
 */
int repo_cournalstore_cursor_close(struct Datastore* datastore, void* cursor);

int journal_record_free(struct JournalRecord* rec);
