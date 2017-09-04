#pragma once
/**
 * Piggyback on the datastore to access the journal entries
 */

#include <stdint.h>
#include <stdlib.h>

#include "lmdb.h"
#include "libp2p/db/datastore.h"

struct JournalRecord {
	unsigned long long timestamp; // the timestamp of the file
	int pin; // true if it is to be stored, false if it is to be deleted
	int pending; // true if we do not have this file yet
	uint8_t *hash; // the hash of the block (file)
	size_t hash_size; // the size of the hash
};

struct JournalRecord* lmdb_journal_record_new();

int lmdb_journal_record_free(struct JournalRecord* rec);

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
int repo_journalstore_cursor_close(struct Datastore* datastore, void* cursor);

int journal_record_free(struct JournalRecord* rec);

int lmdb_journalstore_journal_add(MDB_txn* mdb_txn, struct JournalRecord* record);
