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

#define BUF_SIZE 4096

/**
 * We've received a connection. Find out what they want
 */
void *ipfs_null_connection (void *ptr)
{
    struct null_connection_params *connection_param = NULL;
    struct s_ipfs_routing* routing = NULL;
    struct Stream* stream = NULL;

    connection_param = (struct null_connection_params*) ptr;

    // TODO: multistream + secio + message.
    // TODO: when should we exit the for loop and disconnect?

    stream = libp2p_net_multistream_stream_new(connection_param->socket);

    fprintf(stderr, "Connection %d, count %d\n", connection_param->socket, *(connection_param->count));

	if (libp2p_net_multistream_negotiate(stream)) {
	    routing = ipfs_routing_new_online(connection_param->local_node->repo, &connection_param->local_node->identity->private_key, stream);

		for(;;) {
			struct Libp2pMessage* msg = libp2p_net_multistream_get_message(stream);
			if (msg != NULL) {
				switch(msg->message_type) {
				case (MESSAGE_TYPE_PING):
					routing->Ping(routing, msg);
					break;
				case (MESSAGE_TYPE_GET_VALUE):
					routing->GetValue(routing, msg->key, msg->key_size, NULL, NULL);
					break;
				default:
					break;
				}
			} else {
				break;
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

	if (stream != NULL) {
		stream->close(stream);
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
