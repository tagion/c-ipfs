#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "libp2p/net/p2pnet.h"
#include "libp2p/net/multistream.h"
#include "libp2p/record/message.h"
#include "libp2p/secio/secio.h"
#include "ipfs/repo/fsrepo/fs_repo.h"
#include "ipfs/core/ipfs_node.h"
#include "ipfs/routing/routing.h"
#include "ipfs/importer/resolver.h"
#include "multiaddr/multiaddr.h"

#define BUF_SIZE 4096

int ipfs_ping (int argc, char **argv)
{
	unsigned char* results = NULL;
	size_t results_size = 0;
	int port = 0;
	char* ip = NULL;
	struct SessionContext session;
	struct MultiAddress* address;
	int addressAllocated = 0;
	int retVal = 0;
	struct Libp2pMessage *msg = NULL, *msg_returned = NULL;
	struct IpfsNode local_node;
	unsigned char* protobuf = NULL;
	size_t protobuf_size = 0;
	struct Stream* stream = NULL;

	// sanity check
	if (argc < 3)
		return 0;

    // read the configuration
    struct FSRepo* fs_repo;
	if (!ipfs_repo_fsrepo_new(NULL, NULL, &fs_repo))
		goto exit;

	// open the repository and read the file
	if (!ipfs_repo_fsrepo_open(fs_repo))
		goto exit;

	local_node.identity = fs_repo->config->identity;
	local_node.repo = fs_repo;
	local_node.mode = MODE_ONLINE;
	local_node.routing = ipfs_routing_new_online(&local_node, &fs_repo->config->identity->private_key, stream);
	local_node.peerstore = libp2p_peerstore_new();
	local_node.providerstore = libp2p_providerstore_new();

	if (local_node.routing->Bootstrap(local_node.routing) != 0)
		goto exit;

	if (strstr(argv[2], "/ipfs/") != NULL) {
		// resolve the peer id
		struct Libp2pPeer *peer = ipfs_resolver_find_peer(argv[2], &local_node);
		struct Libp2pLinkedList* current = peer->addr_head;
		// try to find an IP version of the multiaddress
		while (current != NULL) {
			address = (struct MultiAddress*)current->item;
			if (multiaddress_is_ip(address))
				break;
			address = NULL;
		}
	} else {
		// perhaps they passed an IP and port
		if (argc >= 3) {
			char* str = malloc(strlen(argv[2]) + strlen(argv[3]) + 100);
			sprintf(str, "/ip4/%s/tcp/%s", argv[2], argv[3]);
			address  = multiaddress_new_from_string(str);
			free(str);
			if (address != NULL)
				addressAllocated = 1;
		}
		//TODO: Error checking
	}

	if (address == NULL || !multiaddress_is_ip(address)) {
		fprintf(stderr, "Unable to find address\n");
		goto exit;
	}

	if (!multiaddress_get_ip_address(address, &ip)) {
		fprintf(stderr, "Could not convert IP address %s\n", address->string);
		goto exit;
	}
	port = multiaddress_get_ip_port(address);

	session.insecure_stream = libp2p_net_multistream_connect(ip, port);
	session.default_stream = session.insecure_stream;
	if (session.insecure_stream == NULL) {
		fprintf(stderr, "Unable to connect to %s on port %d", ip, port);
		goto exit;
	}

	//TODO: Fix mac problem, then use this to try to switch to secio
	/*
	if (!libp2p_secio_handshake(&session, &fs_repo->config->identity->private_key, 0)) {
		fprintf(stderr, "Unable to switch to secure connection. Attempting insecure ping...\n");
	}
	*/

	// prepare the PING message
	msg = libp2p_message_new();
	msg->message_type = MESSAGE_TYPE_PING;

	protobuf_size = libp2p_message_protobuf_encode_size(msg);
	protobuf = (unsigned char*)malloc(protobuf_size);
	libp2p_message_protobuf_encode(msg, &protobuf[0], protobuf_size, &protobuf_size);
	if (!libp2p_routing_dht_upgrade_stream(&session)) {
		fprintf(stderr, "PING unsuccessful. Unable to switch to kademlia protocol\n");
		goto exit;
	}
	session.default_stream->write(&session, protobuf, protobuf_size);
	session.default_stream->read(&session, &results, &results_size);

	// see if we can unprotobuf
	libp2p_message_protobuf_decode(results, results_size, &msg_returned);
	if (msg_returned->message_type != MESSAGE_TYPE_PING) {
		fprintf(stderr, "Ping unsuccessful. Returned message was not a PING");
		goto exit;
	}

	if (results_size != protobuf_size) {
		fprintf(stderr, "PING unsuccessful. Original size: %lu, returned size: %lu\n", protobuf_size, results_size);
		goto exit;
	}
	if (memcmp(results, protobuf, protobuf_size) != 0) {
		fprintf(stderr, "PING unsuccessful. Results do not match.\n");
		goto exit;
	}

	fprintf(stdout, "Ping of %s:%d successful.\n", ip, port);

	retVal = 1;
	exit:
	if (fs_repo != NULL)
		ipfs_repo_fsrepo_free(fs_repo);
	if (addressAllocated)
		multiaddress_free(address);
	if (ip != NULL)
		free(ip);
	if (msg != NULL)
		libp2p_message_free(msg);
	if (msg_returned != NULL)
		libp2p_message_free(msg_returned);
	if (protobuf != NULL)
		free(protobuf);

	return retVal;

}
