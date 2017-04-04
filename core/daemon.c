#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "libp2p/net/p2pnet.h"
#include "libp2p/peer/peerstore.h"
#include "ipfs/core/daemon.h"
#include "ipfs/core/ipfs_node.h"
#include "ipfs/core/bootstrap.h"
#include "ipfs/repo/fsrepo/fs_repo.h"
#include "libp2p/utils/logger.h"

int ipfs_daemon_start(char* repo_path) {
    int count_pths = 0;
    pthread_t work_pths[MAX];
    struct IpfsNodeListenParams listen_param;

    libp2p_logger_info("daemon", "Initializing daemon...\n");

    // read the configuration
    struct FSRepo* fs_repo;
	if (!ipfs_repo_fsrepo_new(repo_path, NULL, &fs_repo))
		return 0;

	// open the repository and read the file
	if (!ipfs_repo_fsrepo_open(fs_repo)) {
		ipfs_repo_fsrepo_free(fs_repo);
		return 0;
	}

    // create a new IpfsNode
    struct IpfsNode local_node;
    local_node.mode = MODE_ONLINE;
    local_node.peerstore = libp2p_peerstore_new();
	local_node.providerstore = libp2p_providerstore_new();
    local_node.repo = fs_repo;
    local_node.identity = fs_repo->config->identity;

    // Set null router param
    listen_param.ipv4 = 0; // ip 0.0.0.0, all interfaces
    listen_param.port = 4001;
    listen_param.local_node = &local_node;

    // Create pthread for swarm listener.
    if (pthread_create(&work_pths[count_pths++], NULL, ipfs_null_listen, &listen_param)) {
    	libp2p_logger_error("daemon", "Error creating thread for ipfs null listen\n");
        return 1;
    }

    ipfs_bootstrap_routing(&local_node);
    /*
    if (pthread_create(&work_pths[count_pths++], NULL, ipfs_bootstrap_routing, &local_node)) {
    	fprintf(stderr, "Error creating thread for routing\n");
    }
    */

    libp2p_logger_info("daemon", "Daemon is ready\n");

    // Wait for pthreads to finish.
    while (count_pths) {
        if (pthread_join(work_pths[--count_pths], NULL)) {
        	libp2p_logger_error("daemon", "Error joining thread\n");
            return 2;
        }
    }

    // All pthreads aborted?
    return 0;

}

int ipfs_daemon (int argc, char **argv)
{
	libp2p_logger_add_class("peerstore");
	libp2p_logger_add_class("providerstore");
	libp2p_logger_add_class("daemon");
	return ipfs_daemon_start(NULL);
}
