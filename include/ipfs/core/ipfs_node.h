#pragma once

enum NodeMode { MODE_OFFLINE, MODE_ONLINE };

struct IpfsNode {
	enum NodeMode mode;
	//struct PeerId identity;
	struct FSRepo* repo;
	struct Peerstore* peerstore;
	//struct Pinner pinning; // an interface
	//struct Mount** mounts;
	//struct PrivKey* private_key;
	// TODO: Add more here
};
