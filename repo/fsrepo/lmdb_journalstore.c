#include "ipfs/repo/fsrepo/journalstore.h"
#include "ipfs/repo/fsrepo/lmdb_datastore.h"

#include "lmdb.h"
#include "varint.h"
#include <string.h>

int journal_record_free(struct JournalRecord* rec) {
	if (rec != NULL) {
		if (rec->hash != NULL)
			free(rec->hash);
		rec->hash = NULL;
		free(rec);
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
			if (mdb_txn_begin(mdb_env, NULL, 0, &cursor->transaction) != 0)
				return 0;
			MDB_txn* mdb_txn = (MDB_txn*)cursor->transaction;
			if (mdb_dbi_open(mdb_txn, JOURNAL_DB, MDB_DUPSORT | MDB_CREATE, &mdb_dbi) != 0) {
				mdb_txn_commit(mdb_txn);
				return 0;
			}
			// open cursor
			if (mdb_cursor_open(mdb_txn, mdb_dbi, &cursor->cursor) != 0) {
				mdb_txn_commit(mdb_txn);
				return 0;
			}
			return 1;
		}
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
int repo_cournalstore_cursor_close(struct Datastore* datastore, void* crsr) {
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
