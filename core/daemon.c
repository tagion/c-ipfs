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
#include "ipfs/repo/init.h"
#include "libp2p/utils/logger.h"

int ipfs_daemon_start(char* repo_path) {
    int count_pths = 0, retVal = 0;
    pthread_t work_pths[MAX];
    struct IpfsNodeListenParams listen_param;
    struct MultiAddress* ma = NULL;

    libp2p_logger_info("daemon", "Initializing daemon...\n");

    // read the configuration
    struct FSRepo* fs_repo = NULL;
	if (!ipfs_repo_fsrepo_new(repo_path, NULL, &fs_repo))
		goto exit;

	// open the repository and read the file
	if (!ipfs_repo_fsrepo_open(fs_repo)) {
		goto exit;
	}

    // create a new IpfsNode
    struct IpfsNode local_node;
    local_node.mode = MODE_ONLINE;
    local_node.peerstore = libp2p_peerstore_new();
	local_node.providerstore = libp2p_providerstore_new();
    local_node.repo = fs_repo;
    local_node.identity = fs_repo->config->identity;

    // Set null router param
    ma = multiaddress_new_from_string(fs_repo->config->addresses->swarm_head->item);
    listen_param.port = multiaddress_get_ip_port(ma);
    listen_param.ipv4 = 0; // ip 0.0.0.0, all interfaces
    listen_param.local_node = &local_node;

    // Create pthread for swarm listener.
    if (pthread_create(&work_pths[count_pths++], NULL, ipfs_null_listen, &listen_param)) {
    	libp2p_logger_error("daemon", "Error creating thread for ipfs null listen\n");
    	goto exit;
    }

    ipfs_bootstrap_routing(&local_node);

    libp2p_logger_info("daemon", "Daemon is ready\n");

    // Wait for pthreads to finish.
    while (count_pths) {
        if (pthread_join(work_pths[--count_pths], NULL)) {
        	libp2p_logger_error("daemon", "Error joining thread\n");
            goto exit;
        }
    }

    retVal = 1;
    exit:
	fprintf(stderr, "Cleaning up daemon processes\n");
    // clean up
    if (fs_repo != NULL)
    	ipfs_repo_fsrepo_free(fs_repo);
    if (local_node.peerstore != NULL)
    	libp2p_peerstore_free(local_node.peerstore);
    if (local_node.providerstore != NULL)
    	libp2p_providerstore_free(local_node.providerstore);
    if (ma != NULL)
    	multiaddress_free(ma);
    if (local_node.routing != NULL) {
    	ipfs_routing_online_free(local_node.routing);
    }
    return retVal;

}

int ipfs_daemon_stop() {
	return ipfs_null_shutdown();
}

int ipfs_daemon (int argc, char **argv)
{
	char* repo_path = NULL;

	if (!ipfs_repo_get_directory(argc, argv, &repo_path)) {
		fprintf(stderr, "Unable to open repo: %s\n", repo_path);
		return 0;
	}

	libp2p_logger_add_class("peerstore");
	libp2p_logger_add_class("providerstore");
	libp2p_logger_add_class("daemon");
	libp2p_logger_add_class("online");

	return ipfs_daemon_start(repo_path);
}
