/***
 * Here are the wrappers for the lightning database
 * NOTE: In this implementation, the database will contain the base32 encoded value
 * of the multihash key if the file exists on disk.
 */

#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "lmdb.h"
#include "libp2p/utils/logger.h"
#include "libp2p/os/utils.h"
#include "ipfs/repo/fsrepo/lmdb_datastore.h"
#include "ipfs/repo/fsrepo/journalstore.h"
#include "varint.h"

/**
 * Build a "value" section for a datastore record
 * @param timestamp the timestamp
 * @param data the data (usually a base32 of the cid hash)
 * @param data_length the length of data
 * @param result the resultant data object
 * @param result_size the size of the result
 * @returns true(1) on success, otherwise 0
 */
int repo_fsrepo_lmdb_build_record(const unsigned long long timestamp, const uint8_t *data, size_t data_length, uint8_t **result, size_t *result_size) {
	// turn timestamp into varint
	uint8_t ts_varint[8];
	size_t num_bytes;
	if (varint_encode(timestamp, &ts_varint[0], 8, &num_bytes) == NULL) {
		return 0;
	}
	// make new structure
	*result = (uint8_t *) malloc(num_bytes + data_length);
	if (*result == NULL) {
		return 0;
	}
	memcpy(*result, ts_varint, num_bytes);
	memcpy(&(*result)[num_bytes], data, data_length);
	*result_size = data_length + num_bytes;
	return 1;
}

/**
 * read a "value" section from a datastore record.
 * @param data what we read from the datastore
 * @param data_length the length of what we read from the datastore
 * @param timestamp the timestamp that was read from the datastore
 * @param record_pos where the data starts (without the timestamp)
 * @param record_size the size of the data section of the record
 * @returns true(1) on success, false(0) otherwise
 */
int repo_fsrepo_lmdb_parse_record(uint8_t *data, size_t data_length, unsigned long long *timestamp, uint8_t *record_pos, size_t *record_size) {
	size_t varint_size = 0;
	*timestamp = varint_decode(data, data_length, &varint_size);
	record_pos = &data[varint_size];
	return 1;
}

/***
 * retrieve a record from the database and put in a pre-sized buffer
 * @param key the key to look for
 * @param key_size the length of the key
 * @param data the data that is retrieved
 * @param max_data_size the length of the data buffer
 * @param data_size the length of the data that was found in the database
 * @param datastore where to look for the data
 * @returns true(1) on success
 */
int repo_fsrepo_lmdb_get(const char* key, size_t key_size, unsigned char* data, size_t max_data_size, size_t* data_size, const struct Datastore* datastore) {
	MDB_txn* mdb_txn;
	MDB_dbi mdb_dbi;
	struct MDB_val db_key;
	struct MDB_val db_value;

	MDB_env* mdb_env = (MDB_env*)datastore->handle;
	if (mdb_env == NULL)
		return 0;

	// open transaction
	if (mdb_txn_begin(mdb_env, NULL, 0, &mdb_txn) != 0)
		return 0;

	if (mdb_dbi_open(mdb_txn, DATASTORE_DB, MDB_DUPSORT | MDB_CREATE, &mdb_dbi) != 0) {
		mdb_txn_commit(mdb_txn);
		return 0;
	}

	// prepare data
	db_key.mv_size = key_size;
	db_key.mv_data = (char*)key;

	if (mdb_get(mdb_txn, mdb_dbi, &db_key, &db_value) != 0) {
		mdb_txn_commit(mdb_txn);
		return 0;
	}

	// the data from the database includes a timestamp. We'll need to strip it off.
	unsigned long long timestamp;
	uint8_t *pos = NULL;
	size_t size = 0;
	if (!repo_fsrepo_lmdb_parse_record(db_key.mv_data, db_key.mv_size, &timestamp, pos, &size)) {
		mdb_txn_commit(mdb_txn);
		return 0;
	}

	// Was it too big to fit in the buffer they sent?
	if (size > max_data_size) {
		mdb_txn_commit(mdb_txn);
		return 0;
	}

	// set return values
	memcpy(data, pos, size);
	(*data_size) = size;

	// clean up
	mdb_txn_commit(mdb_txn);

	return 1;
}

/**
 * Write data to the datastore with the specified key
 * @param key the key
 * @param key_size the length of the key
 * @param data the data to be written
 * @param data_size the length of the data to be written
 * @param datastore the datastore to write to
 * @returns true(1) on success
 */
int repo_fsrepo_lmdb_put(unsigned const char* key, size_t key_size, unsigned char* data, size_t data_size, const struct Datastore* datastore) {
	int retVal;
	MDB_txn* mdb_txn;
	MDB_dbi mdb_dbi;
	struct MDB_val db_key;
	struct MDB_val db_value;

	MDB_env* mdb_env = (MDB_env*)datastore->handle;
	if (mdb_env == NULL)
		return 0;

	// open transaction
	retVal = mdb_txn_begin(mdb_env, NULL, 0, &mdb_txn);
	if (retVal != 0)
		return 0;
	retVal = mdb_dbi_open(mdb_txn, DATASTORE_DB, MDB_DUPSORT | MDB_CREATE, &mdb_dbi);
	if (retVal != 0)
		return 0;

	// add the timestamp
	unsigned long long timestamp = os_utils_gmtime();
	uint8_t *record;
	size_t record_size;
	repo_fsrepo_lmdb_build_record(timestamp, data, data_size, &record, &record_size);

	// prepare data
	db_key.mv_size = key_size;
	db_key.mv_data = (char*)key;

	// write
	db_value.mv_size = record_size;
	db_value.mv_data = record;

	retVal = mdb_put(mdb_txn, mdb_dbi, &db_key, &db_value, MDB_NODUPDATA | MDB_NOOVERWRITE);
	if (retVal == 0) {
		// the normal case
		lmdb_journalstore_journal_add(mdb_txn, timestamp, key, key_size);
		retVal = 1;
	} else {
		if (retVal == MDB_KEYEXIST) // We tried to add a key that already exists. Skip.
			retVal = 1;
		else
			retVal = 0;
	}

	// cleanup
	free(record);
	mdb_txn_commit(mdb_txn);
	return retVal;
}

/**
 * Open an lmdb database with the given parameters.
 * Note: for now, the parameters are not used
 * @param argc number of parameters in the following array
 * @param argv an array of parameters
 */
int repo_fsrepro_lmdb_open(int argc, char** argv, struct Datastore* datastore) {
	// create environment
	struct MDB_env* mdb_env;
	if (mdb_env_create(&mdb_env) < 0) {
		mdb_env_close(mdb_env);
		return 0;
	}

	// at most, 2 databases will be opened. The datastore and the journal.
	MDB_dbi dbs = 2;
	if (mdb_env_set_maxdbs(mdb_env, dbs) != 0) {
		mdb_env_close(mdb_env);
		return 0;
	}

	// open the environment
	if (mdb_env_open(mdb_env, datastore->path, 0, S_IRWXU) < 0) {
		mdb_env_close(mdb_env);
		return 0;
	}

	datastore->handle = (void*)mdb_env;
	return 1;
}

/***
 * Close an LMDB database
 * NOTE: for now, argc and argv are not used
 * @param argc number of parameters in the argv array
 * @param argv parameters to be passed in
 * @param datastore the datastore struct that contains information about the opened database
 */
int repo_fsrepo_lmdb_close(struct Datastore* datastore) {
	struct MDB_env* mdb_env = (struct MDB_env*)datastore->handle;
	mdb_env_close(mdb_env);
	return 1;
}

/***
 * Create a new cursor on the datastore database
 * @param datastore the place to store the cursor
 * @returns true(1) on success, false(0) otherwise
 */
int repo_fsrepo_lmdb_cursor_open(struct Datastore* datastore) {
	if (datastore->handle != NULL) {
		MDB_env* mdb_env = (MDB_env*)datastore->handle;
		MDB_dbi mdb_dbi;
		if (datastore->cursor == NULL ) {
			datastore->cursor = malloc(sizeof(struct lmdb_trans_cursor));
			struct lmdb_trans_cursor* cursor = (struct lmdb_trans_cursor*)datastore->cursor;
			// open transaction
			if (mdb_txn_begin(mdb_env, NULL, 0, &cursor->transaction) != 0)
				return 0;
			MDB_txn* mdb_txn = (MDB_txn*)cursor->transaction;
			if (mdb_dbi_open(mdb_txn, DATASTORE_DB, MDB_DUPSORT | MDB_CREATE, &mdb_dbi) != 0) {
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

/***
 * Get a record using a cursor
 * @param key the key from the record
 * @param key_length the length of the key
 * @param value the value of the record
 * @param value_length the length of the value
 * @param CURSOR_FIRST or CURSOR_NEXT
 * @param datastore holds the reference to the opened cursor
 * @returns true(1) on success, false(0) otherwise
 */
int repo_fsrepo_lmdb_cursor_get(unsigned char** key, int* key_length,
		unsigned char** value, int* value_length,
		enum DatastoreCursorOp op, struct Datastore* datastore)
{
	if (datastore->cursor != NULL) {
		struct lmdb_trans_cursor* tc = (struct lmdb_trans_cursor*)datastore->cursor;
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
		*key = (unsigned char*)malloc(mdb_key.mv_size);
		memcpy(*key, mdb_key.mv_data, mdb_key.mv_size);
		*key_length = mdb_key.mv_size;
		if (value != NULL) { // don't do this if a null is passed in, time saver
			*value = (unsigned char*)malloc(mdb_value.mv_size);
			memcpy(*value, mdb_value.mv_data, mdb_value.mv_size);
			*value_length = mdb_value.mv_size;
		}
		return 1;
	}
	return 0;
}
/**
 * Close an existing cursor
 * @param datastore the context
 * @returns true(1) on success
 */
int repo_fsrepo_lmdb_cursor_close(struct Datastore* datastore) {
	if (datastore->cursor != NULL) {
		struct lmdb_trans_cursor* cursor = (struct lmdb_trans_cursor*)datastore->cursor;
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

/***
 * Places the LMDB methods into the datastore's function pointers
 * @param datastore the datastore to fill
 * @returns true(1) on success;
 */
int repo_fsrepo_lmdb_cast(struct Datastore* datastore) {
	datastore->datastore_open = &repo_fsrepro_lmdb_open;
	datastore->datastore_close = &repo_fsrepo_lmdb_close;
	datastore->datastore_put = &repo_fsrepo_lmdb_put;
	datastore->datastore_get = &repo_fsrepo_lmdb_get;
	datastore->datastore_cursor_open = &repo_fsrepo_lmdb_cursor_open;
	datastore->datastore_cursor_get = &repo_fsrepo_lmdb_cursor_get;
	datastore->datastore_cursor_close = &repo_fsrepo_lmdb_cursor_close;
	datastore->cursor = NULL;
	return 1;
}

/***
 * Creates the directory
 * @param datastore contains the path that needs to be created
 * @returns true(1) on success
 */
int repo_fsrepo_lmdb_create_directory(struct Datastore* datastore) {
#ifdef __MINGW32__
	return mkdir(datastore->path) == 0;
#else
	return mkdir(datastore->path, S_IRWXU) == 0;
#endif
}

