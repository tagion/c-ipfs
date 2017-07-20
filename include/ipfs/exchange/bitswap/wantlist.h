/**
 * This is a list of requests from a peer.
 * NOTE: This tracks who wants what. If 2 peers want the same file,
 * there will be 1 WantListEntry in the WantList. There will be 2 entries in
 * WantListEntry.sessionsRequesting.
 */

struct WantListEntry {
	unsigned char* cid;
	size_t cid_length;
	int priority;
	struct Libp2pVector* sessionsRequesting;
};

struct WantList {
	struct Libp2pVector* set;
};
