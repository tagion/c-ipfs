#include <string.h>

#include "varint.h"
#include "lmdb.h"
#include "libp2p/utils/logger.h"
#include "ipfs/repo/fsrepo/journalstore.h"
#include "ipfs/repo/fsrepo/lmdb_datastore.h"


int journal_record_free(struct JournalRecord* rec) {
	if (rec != NULL) {
		if (rec->hash != NULL)
			free(rec->hash);
		rec->hash = NULL;
		free(rec);
	}
	return 1;
}

/***
 * Write a journal record
 * @param mbd_txn the transaction
 * @param timestamp the timestamp
 * @param hash the hash
 * @param hash_size the size of the hash
 * @returns true(1) on success, false(0) otherwise
 */
int lmdb_journalstore_journal_add(MDB_txn* mdb_txn, unsigned long long timestamp, const uint8_t *hash, size_t hash_size) {
	MDB_dbi mdb_dbi;
	struct MDB_val db_key;
	struct MDB_val db_value;

	// build the record, which is a timestamp as a key, a byte that is the pin flag, and the hash as the value
	uint8_t time_varint[8];
	size_t time_varint_size = 0;
	varint_encode(timestamp, &time_varint[0], 8, &time_varint_size);

	size_t record_size = hash_size + 1;
	uint8_t record[record_size];
	record[0] = 1;
	memcpy(&record[1], hash, hash_size);

	// open the journal table

	if (mdb_dbi_open(mdb_txn, "JOURNALSTORE", MDB_DUPSORT | MDB_CREATE, &mdb_dbi) != 0) {
		return 0;
	}

	// write the record
	db_key.mv_size = time_varint_size;
	db_key.mv_data = time_varint;

	db_value.mv_size = record_size;
	db_value.mv_data = record;

	if (mdb_put(mdb_txn, mdb_dbi, &db_key, &db_value, 0) == 0) {
		return 0;
	}

	return 1;
}

/**
 * Open a cursor to the journalstore table
 * @param datastore the data connection
 * @param crsr a reference to the cursor. In this implementation, it is an lmdb_trans_cursor struct
 * @returns true(1) on success, false(0) otherwises
 */
int repo_journalstore_cursor_open(struct Datastore* datastore, void** crsr) {
	if (datastore->handle != NULL) {
		MDB_env* mdb_env = (MDB_env*)datastore->handle;
		MDB_dbi mdb_dbi;
		if (*crsr == NULL ) {
			*crsr = malloc(sizeof(struct lmdb_trans_cursor));
			struct lmdb_trans_cursor* cursor = (struct lmdb_trans_cursor*)*crsr;
			// open transaction
			if (mdb_txn_begin(mdb_env, NULL, 0, &cursor->transaction) != 0) {
				libp2p_logger_error("lmdb_journalstore", "Unable to start a transaction.\n");
				return 0;
			}
			MDB_txn* mdb_txn = (MDB_txn*)cursor->transaction;
			if (mdb_dbi_open(mdb_txn, "JOURNALSTORE", MDB_DUPSORT | MDB_CREATE, &mdb_dbi) != 0) {
				libp2p_logger_error("lmdb_journalstore", "Unable to open the dbi to the journalstore");
				mdb_txn_commit(mdb_txn);
				return 0;
			}
			// open cursor
			if (mdb_cursor_open(mdb_txn, mdb_dbi, &cursor->cursor) != 0) {
				mdb_txn_commit(mdb_txn);
				return 0;
			}
			return 1;
		} else {
			libp2p_logger_error("lmdb_journalstore", "Attempted to open a new cursor but there is something already there.\n");
		}
	} else {
		libp2p_logger_error("lmdb_journalstore", "Unable to open cursor on null db handle.\n");
	}
	return 0;

}

/**
 * Read a record from the cursor
 */
int repo_journalstore_cursor_get(struct Datastore* datastore, void* crsr, enum DatastoreCursorOp op, struct JournalRecord** record) {
	if (crsr != NULL) {
		struct lmdb_trans_cursor* tc = (struct lmdb_trans_cursor*)crsr;
		MDB_val mdb_key;
		MDB_val mdb_value;
		MDB_cursor_op co = MDB_FIRST;

		if (op == CURSOR_FIRST)
			co = MDB_FIRST;
		else if (op == CURSOR_NEXT)
			co = MDB_NEXT;
		else if (op == CURSOR_LAST)
			co = MDB_LAST;
		else if (op == CURSOR_PREVIOUS)
			co = MDB_PREV;

		if (mdb_cursor_get(tc->cursor, &mdb_key, &mdb_value, co) != 0) {
			return 0;
		}
		// build the JournalRecord object
		*record = (struct JournalRecord*) malloc(sizeof(struct JournalRecord));
		struct JournalRecord *rec = *record;
		// timestamp
		size_t varint_size = 0;
		rec->timestamp = varint_decode(mdb_key.mv_data, mdb_key.mv_size, &varint_size);
		// pin flag
		rec->pin = ((uint8_t*)mdb_value.mv_data)[0];
		rec->hash_size = mdb_value.mv_size - 1;
		rec->hash = malloc(rec->hash_size);
		memcpy(rec->hash, &mdb_value.mv_data[1], rec->hash_size);
		return 1;
	}
	return 0;
}

/**
 * Close the cursor
 */
int repo_journalstore_cursor_close(struct Datastore* datastore, void* crsr) {
	if (crsr != NULL) {
		struct lmdb_trans_cursor* cursor = (struct lmdb_trans_cursor*)crsr;
		if (cursor->cursor != NULL) {
			mdb_cursor_close(cursor->cursor);
			mdb_txn_commit(cursor->transaction);
			free(cursor);
			return 1;
		}
		free(cursor);
	}
	return 0;
}
