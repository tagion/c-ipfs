#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "libp2p/net/p2pnet.h"
#include "libp2p/record/message.h"
#include "libp2p/net/multistream.h"
#include "ipfs/core/daemon.h"
#include "ipfs/routing/routing.h"
#include "ipfs/core/ipfs_node.h"
#include "libp2p/secio/secio.h"
#include "libp2p/nodeio/nodeio.h"
#include "libp2p/routing/dht_protocol.h"
#include "ipfs/merkledag/merkledag.h"
#include "ipfs/merkledag/node.h"

#define BUF_SIZE 4096

/***
 * Compare incoming to see if they are requesting a protocol upgrade
 * @param incoming the incoming string
 * @param incoming_size the size of the incoming string
 * @param test the protocol string to compare it with (i.e. "/secio" or "/nodeio"
 * @returns true(1) if there was a match, false(0) otherwise
 */
int protocol_compare(unsigned char* incoming, size_t incoming_size, const char* test) {
	int test_size = strlen(test);
	if (incoming_size >= test_size && strncmp((char*)incoming, test, test_size) == 0)
		return 1;
	return 0;
}

/**
 * We've received a connection. Find out what they want
 */
void *ipfs_null_connection (void *ptr)
{
    struct null_connection_params *connection_param = NULL;
    //struct s_ipfs_routing* routing = NULL;

    connection_param = (struct null_connection_params*) ptr;

    // TODO: multistream + secio + message.
    // TODO: when should we exit the for loop and disconnect?

    struct SessionContext session;
    session.insecure_stream = libp2p_net_multistream_stream_new(connection_param->socket);
    session.default_stream = session.insecure_stream;

    fprintf(stderr, "Connection %d, count %d\n", connection_param->socket, *(connection_param->count));

	if (libp2p_net_multistream_negotiate(session.insecure_stream)) {
	    //routing = ipfs_routing_new_online(connection_param->local_node, &connection_param->local_node->identity->private_key, session.insecure_stream);

		for(;;) {
			// check if they're looking for an upgrade (i.e. secio)
			unsigned char* results = NULL;
			size_t bytes_read;
			session.default_stream->read(&session, &results, &bytes_read);
			if (protocol_compare(results, bytes_read, "/secio")) {
				if (!libp2p_secio_handshake(&session, &connection_param->local_node->identity->private_key, 1)) {
					// rejecting connection
					break;
				}
			} else if (protocol_compare(results, bytes_read, "/nodeio")) {
				if (!libp2p_nodeio_handshake(&session))
					break;
				// loop through file requests
				int _continue = 1;
				while(_continue) {
					unsigned char* hash;
					size_t hash_length = 0;
					_continue = session.default_stream->read(&session, &hash, &hash_length);
					if (hash_length < 20) {
						_continue = 0;
						continue;
					}
					else {
						// try to get the Node
						struct Node* node = NULL;
						if (!ipfs_merkledag_get(hash, hash_length, &node, connection_param->local_node->repo)) {
							_continue = 0;
							continue;
						}
						size_t results_size = ipfs_node_protobuf_encode_size(node);
						unsigned char results[results_size];
						if (!ipfs_node_protobuf_encode(node, results, results_size, &results_size)) {
							_continue = 0;
							continue;
						}
						// send it to the requestor
						session.default_stream->write(&session, results, results_size);
					}
				}
			} else if (protocol_compare(results, bytes_read, "/kad/")) {
				if (!libp2p_routing_dht_handshake(&session))
					break;
				// this handles 1 transaction
				libp2p_routing_dht_handle_message(&session, connection_param->local_node->peerstore, connection_param->local_node->providerstore);
			}
			else {
				// oops there was a problem
				//TODO: Handle this
				/*
				struct Libp2pMessage* msg = NULL;
				libp2p_message_protobuf_decode(results, bytes_read, &msg);
				if (msg != NULL) {
					switch(msg->message_type) {
					case (MESSAGE_TYPE_PING):
						routing->Ping(routing, msg);
						break;
					case (MESSAGE_TYPE_GET_VALUE): {
						unsigned char* val;
						size_t val_size = 0;
						routing->GetValue(routing, msg->key, msg->key_size, (void**)&val, &val_size);
						if (val == NULL) {
							// write a 0 to the stream to tell the client we couldn't find it.
							session.default_stream->write(&session, 0, 1);
						} else {
							session.default_stream->write(&session, val, val_size);
						}
						break;
					}
					default:
						break;
					}
				} else {
					break;
				}
				*/
			}
		}
   	}
	/*
	len = socket_read(connection_param->socket, b, sizeof(b)-1, 0);
	if (len > 0) {
		while (b[len-1] == '\r' || b[len-1] == '\n') len--;
		b[len] = '\0';
		fprintf(stderr, "Recv: '%s'\n", b);
		if (strcmp (b, "ping") == 0) {
			socket_write(connection_param->socket, "pong", 4, 0);
		}
	} else if(len < 0) {
		break;
	}
	*/

	if (session.default_stream != NULL) {
		session.default_stream->close(&session);
	}
    (*(connection_param->count))--; // update counter.
    free (connection_param);
    return (void*) 1;
}

void *ipfs_null_listen (void *ptr)
{
    int socketfd, s, count = 0;
    pthread_t pth_connection;
    struct IpfsNodeListenParams *listen_param;
    struct null_connection_params *connection_param;

    listen_param = (struct IpfsNodeListenParams*) ptr;

    if ((socketfd = socket_listen(socket_tcp4(), &(listen_param->ipv4), &(listen_param->port))) <= 0) {
        perror("fail to init null router.");
        exit (1);
    }

    fprintf(stderr, "Null listening on %d\n", listen_param->port);

    for (;;) {
        s = socket_accept4(socketfd, &(listen_param->ipv4), &(listen_param->port));
        if (count >= CONNECTIONS) { // limit reached.
            close (s);
            continue;
        }

        count++;
        connection_param = malloc (sizeof (struct null_connection_params));
        if (connection_param) {
            connection_param->socket = s;
            connection_param->count = &count;
            connection_param->local_node = listen_param->local_node;
            // Create pthread for ipfs_null_connection.
            if (pthread_create(&pth_connection, NULL, ipfs_null_connection, connection_param)) {
                fprintf(stderr, "Error creating thread for connection %d\n", count);
                close (s);
            } else {
                pthread_detach (pth_connection);
            }
        }
    }

    return (void*) 2;
}
