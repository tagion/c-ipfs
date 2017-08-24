/**
 * The journal protocol attempts to keep a journal in sync with other (approved) nodes
 */

#include "ipfs/journal/journal.h"

/***
 * See if we can handle this message
 * @param incoming the incoming message
 * @param incoming_size the size of the incoming message
 * @returns true(1) if the protocol in incoming is something we can handle. False(0) otherwise.
 */
int ipfs_journal_can_handle(const uint8_t* incoming, size_t incoming_size) {
	if (incoming_size < 8)
		return 0;
	char* result = strstr((char*)incoming, "/ipfs/journal/1.0.0");
	if(result == NULL || result != (char*)incoming)
		return 0;
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
 * Handles a message
 * @param incoming the message
 * @param incoming_size the size of the message
 * @param session_context details of the remote peer
 * @param protocol_context in this case, an IpfsNode
 * @returns 0 if the caller should not continue looping, <0 on error, >0 on success
 */
int ipfs_journal_handle_message(const uint8_t* incoming, size_t incoming_size, struct SessionContext* session_context, void* protocol_context) {
	//struct IpfsNode* local_node = (struct IpfsNode*)protocol_context;
	//TODO: handle the message
	return -1;
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
 * Send a journal message to a remote peer
 * @param peer the peer to send it to
 * @returns true(1) on success, false(0) otherwise.
 */
int ipfs_journal_sync(struct Libp2pPeer* peer) {
	// make sure we're connected securely
	if (peer->is_local)
		return 0;
	if (peer->sessionContext->secure_stream == NULL)
		return 0;
	/*
	// grab the last 10 files
	struct Libp2pVector* vector = libp2p_utils_vector_new(1);
	if (vector == NULL) {
		return 0;
	}
	ipfs_journal_get_last(10, &vector);
	struct JournalMessage* message = NULL;
	// build the message
	if (!ipfs_journal_build_message(message))
		return 0;
	// protobuf the message
	// send the protocol header
	// send the message
	 */
	return 0;
}

