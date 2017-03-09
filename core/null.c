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

#define BUF_SIZE 4096

int ipfs_null_requesting_secio(unsigned char* buffer, size_t buffer_size) {
	if (buffer_size > 5 && strncmp((char*)buffer, "/secio", 6) == 0)
		return 1;
	return 0;
}
/**
 * We've received a connection. Find out what they want
 */
void *ipfs_null_connection (void *ptr)
{
    struct null_connection_params *connection_param = NULL;
    struct s_ipfs_routing* routing = NULL;

    connection_param = (struct null_connection_params*) ptr;

    // TODO: multistream + secio + message.
    // TODO: when should we exit the for loop and disconnect?

    struct SecureSession secure_session;
    secure_session.insecure_stream = libp2p_net_multistream_stream_new(connection_param->socket);
    secure_session.default_stream = secure_session.insecure_stream;

    fprintf(stderr, "Connection %d, count %d\n", connection_param->socket, *(connection_param->count));

	if (libp2p_net_multistream_negotiate(secure_session.insecure_stream)) {
	    routing = ipfs_routing_new_online(connection_param->local_node, &connection_param->local_node->identity->private_key, secure_session.insecure_stream);

		for(;;) {
			// check if they're looking for an upgrade (i.e. secio)
			unsigned char* results = NULL;
			size_t bytes_read;
			secure_session.default_stream->read(&secure_session, &results, &bytes_read);
			if (ipfs_null_requesting_secio(results, bytes_read)) {
				if (!libp2p_secio_handshake(&secure_session, &connection_param->local_node->identity->private_key, 1)) {
					// rejecting connection
					break;
				}
			} else {
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
							secure_session.default_stream->write(&secure_session, 0, 1);
						} else {
							secure_session.default_stream->write(&secure_session, val, val_size);
						}
						break;
					}
					default:
						break;
					}
				} else {
					break;
				}
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

	if (secure_session.default_stream != NULL) {
		secure_session.default_stream->close(&secure_session);
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
