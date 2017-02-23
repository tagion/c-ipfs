#pragma once

#include "ipfs/repo/config/identity.h"
#include "ipfs/repo/fsrepo/fs_repo.h"
#include "libp2p/peer/peerstore.h"

enum NodeMode { MODE_OFFLINE, MODE_ONLINE };

struct IpfsNode {
	enum NodeMode mode;
	struct Identity* identity;
	struct FSRepo* repo;
	struct Peerstore* peerstore;
	//struct Pinner pinning; // an interface
	//struct Mount** mounts;
	// TODO: Add more here
};
