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

/***
 * Starts a server with a file in it with a specific set of files.
 * It also has a specific address, with a config file of another
 * known peer to replicate between
 */
int test_journal_server_1() {
	int retVal = 0;
	pthread_t daemon_thread;
	int thread_started = 0;
	char* ipfs_path = "/tmp/ipfs_1";
	char* config_file = "config.test1";
	struct FSRepo* fs_repo = NULL;

	libp2p_logger_add_class("test_journal");
	libp2p_logger_add_class("journal");
	libp2p_logger_add_class("daemon");
	libp2p_logger_add_class("online");
	libp2p_logger_add_class("peer");
	//libp2p_logger_add_class("null");
	libp2p_logger_add_class("replication");
	libp2p_logger_add_class("fs_repo");
	libp2p_logger_add_class("lmdb_journalstore");
	libp2p_logger_add_class("secio");
	libp2p_logger_add_class("socket");

	if (!drop_build_open_repo(ipfs_path, &fs_repo, config_file)) {
		ipfs_repo_fsrepo_free(fs_repo);
		libp2p_logger_error("test_journal", "Unable to drop and build repository at %s\n", ipfs_path);
		goto exit;
	}

	libp2p_logger_debug("test_journal", "Changed the server id to %s.\n", fs_repo->config->identity->peer->id);

	ipfs_repo_fsrepo_free(fs_repo);

	// add some files to the datastore
	uint8_t *bytes = (unsigned char*)"hello, world!\n";
	char* filename = "test1.txt";
	create_file(filename, bytes, strlen((char*)bytes));
	struct HashtableNode* node;
	size_t bytes_written;
	struct IpfsNode *local_node = NULL;
	ipfs_node_offline_new(ipfs_path, &local_node);
	ipfs_import_file(NULL, filename, &node, local_node, &bytes_written, 0);
	ipfs_node_free(local_node);

	libp2p_logger_debug("test_journal", "*** Firing up daemon for server 2 ***\n");

	pthread_create(&daemon_thread, NULL, test_daemon_start, (void*)ipfs_path);
	thread_started = 1;

	sleep(45);

	retVal = 1;
	exit:
	ipfs_daemon_stop();
	if (thread_started)
		pthread_join(daemon_thread, NULL);
	return retVal;
}

/***
 * Starts a server with a specific set of files.
 * It also has a specific address, with a config file of another
 * known peer to replicate between
 */
int test_journal_server_2() {
	int retVal = 0;
	pthread_t daemon_thread;
	int thread_started = 0;
	char* ipfs_path = "/tmp/ipfs_2";
	char* config_file = "config.test2";
	struct FSRepo* fs_repo = NULL;

	libp2p_logger_add_class("test_journal");
	libp2p_logger_add_class("journal");
	libp2p_logger_add_class("daemon");
	libp2p_logger_add_class("online");
	libp2p_logger_add_class("peer");
	//libp2p_logger_add_class("null");
	libp2p_logger_add_class("replication");
	libp2p_logger_add_class("fs_repo");
	libp2p_logger_add_class("lmdb_journalstore");
	libp2p_logger_add_class("secio");

	if (!drop_build_open_repo(ipfs_path, &fs_repo, config_file)) {
		ipfs_repo_fsrepo_free(fs_repo);
		libp2p_logger_error("test_journal", "Unable to drop and build repository at %s\n", ipfs_path);
		goto exit;
	}

	libp2p_logger_debug("test_journal", "Changed the server id to %s.\n", fs_repo->config->identity->peer->id);

	ipfs_repo_fsrepo_free(fs_repo);

	libp2p_logger_debug("test_journal", "*** Firing up daemon for server 2 ***\n");

	pthread_create(&daemon_thread, NULL, test_daemon_start, (void*)ipfs_path);
	thread_started = 1;

	sleep(30);

	retVal = 1;
	exit:
	ipfs_daemon_stop();
	if (thread_started)
		pthread_join(daemon_thread, NULL);
	return retVal;
}
