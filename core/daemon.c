#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "libp2p/net/p2pnet.h"
#include "ipfs/core/daemon.h"

int ipfs_daemon (int argc, char **argv)
{
    int count_pths = 0;
    pthread_t work_pths[MAX];
    struct null_listen_params listen_param;

    fprintf(stderr, "Initializing daemon...\n");

    // Set null router param
    listen_param.ipv4 = 0; // ip 0.0.0.0, all interfaces
    listen_param.port = 4001;

    // Create pthread for ipfs_null_listen.
    if (pthread_create(&work_pths[count_pths++], NULL, ipfs_null_listen, &listen_param)) {
        fprintf(stderr, "Error creating thread for ipfs_null_listen\n");
        return 1;
    }

    fprintf(stderr, "Daemon is ready\n");

    // Wait for pthreads to finish.
    while (count_pths) {
        if (pthread_join(work_pths[--count_pths], NULL)) {
            fprintf(stderr, "Error joining thread\n");
            return 2;
        }
    }

    // All pthreads aborted?
    return 0;
}
