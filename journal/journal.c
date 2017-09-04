/**
 * The journal protocol attempts to keep a journal in sync with other (approved) nodes
 */
#include "libp2p/crypto/encoding/base58.h"
#include "libp2p/os/utils.h"
#include "libp2p/utils/logger.h"
#include "ipfs/journal/journal.h"
#include "ipfs/journal/journal_message.h"
#include "ipfs/journal/journal_entry.h"
#include "ipfs/repo/fsrepo/journalstore.h"
#include "ipfs/repo/config/replication.h"

/***
 * See if we can handle this message
 * @param incoming the incoming message
 * @param incoming_size the size of the incoming message
 * @returns true(1) if the protocol in incoming is something we can handle. False(0) otherwise.
 */
int ipfs_journal_can_handle(const uint8_t* incoming, size_t incoming_size) {
	if (incoming_size < 8)
		return 0;
	char* result = strstr((char*)incoming, "/ipfs/journalio/1.0.0");
	if(result == NULL || result != (char*)incoming)
		return 0;
	libp2p_logger_debug("journal", "Handling incoming message.\n");
	return 1;
}

/**
 * Clean up resources used by this handler
 * @param context the context to clean up
 * @returns true(1)
 */
int ipfs_journal_shutdown_handler(void* context) {
	return 1;
}

/***
 * Build the protocol handler struct for the Journal protocol
 * @param local_node what to stuff in the context
 * @returns the protocol handler
 */
struct Libp2pProtocolHandler* ipfs_journal_build_protocol_handler(const struct IpfsNode* local_node) {
	struct Libp2pProtocolHandler* handler = (struct Libp2pProtocolHandler*) malloc(sizeof(struct Libp2pProtocolHandler));
	if (handler != NULL) {
		handler->context = (void*)local_node;
		handler->CanHandle = ipfs_journal_can_handle;
		handler->HandleMessage = ipfs_journal_handle_message;
		handler->Shutdown = ipfs_journal_shutdown_handler;
	}
	return handler;
}

/***
 * Retrieve the last n records from the journalstore
 * @param database the reference to the opened db
 * @param n the number of records to retrieve
 * @returns a vector of struct JournalRecord
 */
struct Libp2pVector* ipfs_journal_get_last(struct Datastore* database, int n) {
	struct Libp2pVector* vector = libp2p_utils_vector_new(1);
	if (vector != NULL) {
		void* cursor = NULL;
		if (!repo_journalstore_cursor_open(database, &cursor)) {
			libp2p_logger_error("journal", "Unable to open a cursor for the journalstore.\n");
			return NULL;
		}
		struct JournalRecord* rec = NULL;
		if (!repo_journalstore_cursor_get(database, cursor, CURSOR_LAST, &rec)) {
			libp2p_logger_error("journal", "Unable to find last record from the journalstore.\n");
			libp2p_utils_vector_free(vector);
			repo_journalstore_cursor_close(database, cursor);
			return NULL;
		}
		// we've got one, now start the loop
		int i = 0;
		do {
			libp2p_logger_debug("journal", "Adding record to the vector.\n");
			libp2p_utils_vector_add(vector, rec);
			if (!repo_journalstore_cursor_get(database, cursor, CURSOR_PREVIOUS, &rec)) {
				break;
			}
			i++;
		} while(i < n);
		libp2p_logger_debug("journal", "Closing journalstore cursor.\n");
		repo_journalstore_cursor_close(database, cursor);
	} else {
		libp2p_logger_error("journal", "Unable to allocate vector for ipfs_journal_get_last.\n");
	}
	return vector;
}

int ipfs_journal_free_records(struct Libp2pVector* records) {
	if (records != NULL) {
		for (int i = 0; i < records->total; i++) {
			struct JournalRecord* rec = (struct JournalRecord*)libp2p_utils_vector_get(records, i);
			journal_record_free(rec);
		}
		libp2p_utils_vector_free(records);
	}
	return 1;
}

int ipfs_journal_send_message(struct IpfsNode* node, struct Libp2pPeer* peer, struct JournalMessage* message) {
	if (peer->connection_type != CONNECTION_TYPE_CONNECTED)
		libp2p_peer_connect(&node->identity->private_key, peer, node->peerstore, node->repo->config->datastore, 10);
	if (peer->connection_type != CONNECTION_TYPE_CONNECTED)
		return 0;
	// protobuf the message
	size_t msg_size = ipfs_journal_message_encode_size(message);
	uint8_t msg[msg_size];
	if (!ipfs_journal_message_encode(message, &msg[0], msg_size, &msg_size))
		return 0;
	// send the header
	char* header = "/ipfs/journalio/1.0.0\n";
	if (!peer->sessionContext->default_stream->write(peer->sessionContext, (unsigned char*)header, strlen(header)))
		return 0;
	// send the message
	return peer->sessionContext->default_stream->write(peer->sessionContext, msg, msg_size);
}

/***
 * Send a journal message to a remote peer
 * @param replication_peer the peer to send it to
 * @returns true(1) on success, false(0) otherwise.
 */
int ipfs_journal_sync(struct IpfsNode* local_node, struct ReplicationPeer* replication_peer) {
	libp2p_logger_debug("journal", "Attempting replication for peer %s.\n", libp2p_peer_id_to_string(replication_peer->peer));
	// get the real peer object from the peersstore
	struct Libp2pPeer *peer = libp2p_peerstore_get_peer(local_node->peerstore, (unsigned char*)replication_peer->peer->id, replication_peer->peer->id_size);
	if (peer == NULL) {
		libp2p_logger_error("journal", "Unable to find peer %s in peerstore.\n", libp2p_peer_id_to_string(replication_peer->peer));
		return 0;
	}
	// make sure we're connected securely
	if (peer->is_local) {
		libp2p_logger_debug("journal", "Cannot replicate a local peer.\n");
		return 0;
	}
	if (peer->sessionContext == NULL || peer->sessionContext->secure_stream == NULL) {
		libp2p_logger_debug("journal", "Cannot replicate over an insecure stream.\n");
		return 0;
	}

	// grab the last 10? files
	struct Libp2pVector* journal_records = ipfs_journal_get_last(local_node->repo->config->datastore, 10);
	if (journal_records == NULL || journal_records->total == 0) {
		// nothing to do
		libp2p_logger_debug("journal", "There are no journal records to process.\n");
		return 1;
	}
	// build the message
	struct JournalMessage* message = ipfs_journal_message_new();
	for(int i = 0; i < journal_records->total; i++) {
		struct JournalRecord* rec = (struct JournalRecord*) libp2p_utils_vector_get(journal_records, i);
		if (rec->timestamp > message->end_epoch)
			message->end_epoch = rec->timestamp;
		if (message->start_epoch == 0 || rec->timestamp < message->start_epoch)
			message->start_epoch = rec->timestamp;
		struct JournalEntry* entry = ipfs_journal_entry_new();
		entry->timestamp = rec->timestamp;
		entry->pin = 1;
		entry->hash_size = rec->hash_size;
		entry->hash = (uint8_t*) malloc(entry->hash_size);
		if (entry->hash == NULL) {
			// out of memory
			ipfs_journal_message_free(message);
			ipfs_journal_free_records(journal_records);
			return 0;
		}
		memcpy(entry->hash, rec->hash, entry->hash_size);
		// debugging
		size_t b58size = 100;
		uint8_t *b58key = (uint8_t*) malloc(b58size);
		libp2p_crypto_encoding_base58_encode(entry->hash, entry->hash_size, &b58key, &b58size);
		free(b58key);
		libp2p_logger_debug("journal", "Adding hash %s to JournalMessage.\n", b58key);
		libp2p_utils_vector_add(message->journal_entries, entry);
	}
	// send the message
	message->current_epoch = os_utils_gmtime();
	libp2p_logger_debug("journal", "Sending message to %s.\n", peer->id);
	int retVal = ipfs_journal_send_message(local_node, peer, message);
	if (retVal) {
		replication_peer->lastConnect = message->current_epoch;
		replication_peer->lastJournalTime = message->end_epoch;
	}
	// clean up
	ipfs_journal_message_free(message);
	ipfs_journal_free_records(journal_records);

	return retVal;
}

enum JournalAction { JOURNAL_ENTRY_NEEDED, JOURNAL_TIME_ADJUST, JOURNAL_REMOTE_NEEDS };

struct JournalToDo {
	enum JournalAction action; // what needs to be done
	unsigned long long local_timestamp; // what we have in our journal
	unsigned long long remote_timestamp; // what they have in their journal
	uint8_t* hash; // the hash
	size_t hash_size; // the size of the hash
};

struct JournalToDo* ipfs_journal_todo_new() {
	struct JournalToDo* j = (struct JournalToDo*) malloc(sizeof(struct JournalToDo));
	if (j != NULL) {
		j->action = JOURNAL_ENTRY_NEEDED;
		j->hash = NULL;
		j->hash_size = 0;
		j->local_timestamp = 0;
		j->remote_timestamp = 0;
	}
	return j;
}

int ipfs_journal_todo_free(struct JournalToDo *in) {
	if (in != NULL) {
		free(in);
	}
	return 1;
}

/***
 * Loop through the incoming message, looking for what may need to change
 * @param local_node the context
 * @param incoming the incoming JournalMessage
 * @param todo_vector a Libp2pVector that gets allocated and filled with JournalToDo structs
 * @returns true(1) on success, false(0) otherwise
 */
int ipfs_journal_build_todo(struct IpfsNode* local_node, struct JournalMessage* incoming, struct Libp2pVector** todo_vector) {
	*todo_vector = libp2p_utils_vector_new(1);
	if (*todo_vector == NULL)
		return -1;
	struct Libp2pVector *todos = *todo_vector;
	// for every file in message
	for(int i = 0; i < incoming->journal_entries->total; i++) {
		struct JournalEntry* entry = (struct JournalEntry*) libp2p_utils_vector_get(incoming->journal_entries, i);
		// do we have the file?
		struct DatastoreRecord *datastore_record = NULL;
		if (!local_node->repo->config->datastore->datastore_get(entry->hash, entry->hash_size, &datastore_record, local_node->repo->config->datastore)) {
			struct JournalToDo* td = ipfs_journal_todo_new();
			td->action = JOURNAL_ENTRY_NEEDED;
			td->hash = entry->hash;
			td->hash_size = entry->hash_size;
			td->remote_timestamp = entry->timestamp;
			libp2p_utils_vector_add(todos, td);
		} else {
			// do we need to adjust the time?
			if (datastore_record->timestamp != entry->timestamp) {
				struct JournalToDo* td = ipfs_journal_todo_new();
				td->action = JOURNAL_TIME_ADJUST;
				td->hash = entry->hash;
				td->hash_size = entry->hash_size;
				td->local_timestamp = datastore_record->timestamp;
				td->remote_timestamp = entry->timestamp;
				libp2p_utils_vector_add(todos, td);
			}
		}
		libp2p_datastore_record_free(datastore_record);
	}
	// TODO: get all files of same second
	// are they perhaps missing something?
	//struct Libp2pVector* local_records_for_second;
	return 0;
}

/***
 * Handles a message
 * @param incoming the message
 * @param incoming_size the size of the message
 * @param session_context details of the remote peer
 * @param protocol_context in this case, an IpfsNode
 * @returns 0 if the caller should not continue looping, <0 on error, >0 on success
 */
int ipfs_journal_handle_message(const uint8_t* incoming, size_t incoming_size, struct SessionContext* session_context, void* protocol_context) {
	// remove protocol
	uint8_t *incoming_pos = (uint8_t*) incoming;
	size_t pos_size = incoming_size;
	int second_read = 0;
	for(int i = 0; i < incoming_size; i++) {
		if (incoming[i] == '\n') {
			if (incoming_size > i + 1) {
				incoming_pos = (uint8_t *)&incoming[i+1];
				pos_size = incoming_size - i;
				break;
			} else {
				// read next segment from network
				if (!session_context->default_stream->read(session_context, &incoming_pos, &pos_size, 10))
					return -1;
				second_read = 1;
			}
		}
	}
	libp2p_logger_debug("journal", "Handling incoming message from %s.\n", session_context->remote_peer_id);
	struct IpfsNode* local_node = (struct IpfsNode*)protocol_context;
	// un-protobuf the message
	struct JournalMessage* message = NULL;
	if (!ipfs_journal_message_decode(incoming_pos, pos_size, &message))
		return -1;
	// see if the remote's time is within 5 minutes of now
	unsigned long long start_time = os_utils_gmtime();
	long long our_time_diff = start_time - message->current_epoch;
	// NOTE: If our_time_diff is negative, the remote's clock is faster than ours.
	// if it is positive, our clock is faster than theirs.
	if ( llabs(our_time_diff) > 300) {
		libp2p_logger_error("journal", "The clock of peer %s is out of 5 minute range. Seconds difference: %llu", session_context->remote_peer_id, our_time_diff);
		if (second_read) {
			free(incoming_pos);
		}
		return -1;
	}
	struct Libp2pVector* todo_vector = NULL;
	ipfs_journal_build_todo(local_node, message, &todo_vector);
	// loop through todo items, and do the right thing
	for(int i = 0; i < todo_vector->total; i++) {
		struct JournalToDo *curr = (struct JournalToDo*) libp2p_utils_vector_get(todo_vector, i);
		switch (curr->action) {
			case (JOURNAL_ENTRY_NEEDED): {
				// go get a file
				struct Block* block = NULL;
				struct Cid* cid = ipfs_cid_new(0, curr->hash, curr->hash_size, CID_PROTOBUF);
				// debugging
				char* str = NULL;
				libp2p_logger_debug("journal", "Looking for block %s.\n", ipfs_cid_to_string(cid, &str));
				if (str != NULL)
					free(str);

				if (local_node->exchange->GetBlockAsync(local_node->exchange, cid, &block)) {
					// set timestamp
				}
				ipfs_cid_free(cid);
				ipfs_block_free(block);
			}
			break;
			case (JOURNAL_TIME_ADJUST): {

			}
			break;
			case (JOURNAL_REMOTE_NEEDS): {

			}
			break;
		}
	}
	//TODO: set new values in their ReplicationPeer struct

	if (second_read)
		free(incoming_pos);
	return 1;
}

