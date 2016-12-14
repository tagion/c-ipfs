/***
 * Here are the wrappers for the lightning database
 * NOTE: In this implementation, the database will contain the base32 encoded value
 * of the multihash key if the file exists on disk.
 */

#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "lmdb.h"
#include "ipfs/repo/fsrepo/lmdb_datastore.h"

int repo_fsrepo_lmdb_get_block(const struct Cid* cid, struct Block** block, const struct Datastore* datastore) {
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
	retVal = mdb_dbi_open(mdb_txn, NULL, MDB_DUPSORT, &mdb_dbi);
	if (retVal != 0) {
		mdb_txn_commit(mdb_txn);
		return 0;
	}

	// prepare data
	db_key.mv_size = cid->hash_length;
	db_key.mv_data = cid->hash;

	retVal = mdb_get(mdb_txn, mdb_dbi, &db_key, &db_value);
	if (retVal != 0) {
		mdb_dbi_close(mdb_env, mdb_dbi);
		mdb_txn_commit(mdb_txn);
		return 0;
	}

	// now copy the data
	retVal = ipfs_blocks_block_new(block);
	if (retVal == 0) {
		mdb_dbi_close(mdb_env, mdb_dbi);
		mdb_txn_commit(mdb_txn);
		return 0;
	}
	retVal = ipfs_blocks_block_add_data(db_value.mv_data, db_value.mv_size, *block);

	// clean up
	mdb_dbi_close(mdb_env, mdb_dbi);
	mdb_txn_commit(mdb_txn);

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
	retVal = mdb_dbi_open(mdb_txn, NULL, MDB_DUPSORT, &mdb_dbi);
	if (retVal != 0) {
		mdb_txn_commit(mdb_txn);
		return 0;
	}

	// prepare data
	db_key.mv_size = key_size;
	db_key.mv_data = (char*)key;

	retVal = mdb_get(mdb_txn, mdb_dbi, &db_key, &db_value);
	if (retVal != 0) {
		mdb_dbi_close(mdb_env, mdb_dbi);
		mdb_txn_commit(mdb_txn);
		return 0;
	}

	// now copy the data
	if (db_value.mv_size > max_data_size) {
		mdb_dbi_close(mdb_env, mdb_dbi);
		mdb_txn_commit(mdb_txn);
		return 0;
	}

	// set return values
	memcpy(data, db_value.mv_data, db_value.mv_size);
	(*data_size) = db_value.mv_size;

	// clean up
	mdb_dbi_close(mdb_env, mdb_dbi);
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
	retVal = mdb_dbi_open(mdb_txn, NULL, MDB_DUPSORT, &mdb_dbi);
	if (retVal != 0)
		return 0;

	// prepare data
	db_key.mv_size = key_size;
	db_key.mv_data = (char*)key;

	// write
	db_value.mv_size = data_size;
	db_value.mv_data = data;
	retVal = mdb_put(mdb_txn, mdb_dbi, &db_key, &db_value, MDB_NODUPDATA | MDB_NOOVERWRITE);
	if (retVal == 0) // the normal case
		retVal = 1;
	else {
		if (retVal == MDB_KEYEXIST) // We tried to add a key that already exists. Skip.
			retVal = 1;
		else
			retVal = 0;
	}

	// cleanup
	mdb_dbi_close(mdb_env, mdb_dbi);
	mdb_txn_commit(mdb_txn);
	return retVal;
}

/**
 * Write a block to the datastore with the specified key
 * @param block the block to be written
 * @param datastore the datastore to write to
 * @returns true(1) on success
 */
int repo_fsrepo_lmdb_put_block(const struct Block* block, const struct Datastore* datastore) {
	return repo_fsrepo_lmdb_put(block->cid->hash, block->cid->hash_length, block->data, block->data_length, datastore);
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
	int retVal = mdb_env_create(&mdb_env);
	if (retVal < 0) {
		mdb_env_close(mdb_env);
		return 0;
	}

	// open the environment
	retVal = mdb_env_open(mdb_env, datastore->path, 0, S_IRWXU);
	if (retVal < 0) {
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
 * Places the LMDB methods into the datastore's function pointers
 * @param datastore the datastore to fill
 * @returns true(1) on success;
 */
int repo_fsrepo_lmdb_cast(struct Datastore* datastore) {
	datastore->datastore_open = &repo_fsrepro_lmdb_open;
	datastore->datastore_close = &repo_fsrepo_lmdb_close;
	datastore->datastore_put = &repo_fsrepo_lmdb_put;
	datastore->datastore_put_block = &repo_fsrepo_lmdb_put_block;
	datastore->datastore_get = &repo_fsrepo_lmdb_get;
	datastore->datastore_get_block = &repo_fsrepo_lmdb_get_block;
	return 1;
}

/***
 * Creates the directory
 * @param datastore contains the path that needs to be created
 * @returns true(1) on success
 */
int repo_fsrepo_lmdb_create_directory(struct Datastore* datastore) {
	return mkdir(datastore->path, S_IRWXU) == 0;
}

