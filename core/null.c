#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "libp2p/net/p2pnet.h"
#include "ipfs/core/daemon.h"

void *ipfs_null_connection (void *ptr)
{
    struct null_connection_params *connection_param;

    connection_param = (struct null_connection_params*) ptr;

    // TODO: multistream + secio + message.
    fprintf(stderr, "Connection %d, count %d\n", connection_param->socket, *(connection_param->count));

    close (connection_param->socket); // close socket.
    (*(connection_param->count))--; // update counter.
    free (connection_param);
    return (void*) 1;
}

void *ipfs_null_listen (void *ptr)
{
    int socketfd, s, count = 0;
    pthread_t pth_connection;
    struct null_listen_params *listen_param;
    struct null_connection_params *connection_param;

    listen_param = (struct null_listen_params*) ptr;

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
