#pragma once

#include "lmdb.h"

struct lmdb_trans_cursor {
	MDB_txn* transaction;
	MDB_dbi* database;
	MDB_cursor* cursor;
};

/**
 * Create a new lmdb_trans_cursor struct
 * @returns a newly allocated trans_cursor struct
 */
struct lmdb_trans_cursor* lmdb_trans_cursor_new();

/***
 * Clean up resources from a lmdb_trans_cursor struct
 * @param in the cursor
 * @returns true(1)
 */
int lmdb_trans_cursor_free(struct lmdb_trans_cursor* in);
