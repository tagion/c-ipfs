#include <stdlib.h>

#include "../test_helper.h"
#include "ipfs/routing/routing.h"
#include "ipfs/repo/fsrepo/fs_repo.h"

void stop_kademlia(void);

int test_routing_supernode_start() {
	int retVal = 0;
	struct FSRepo* fs_repo = NULL;
	struct IpfsNode* ipfs_node = NULL;
	struct Stream* stream = NULL;

	if (!drop_build_and_open_repo("/tmp/.ipfs", &fs_repo))
		goto exit;

	ipfs_node = (struct IpfsNode*)malloc(sizeof(struct IpfsNode));
	ipfs_node->mode = MODE_ONLINE;
	ipfs_node->identity = fs_repo->config->identity;
	ipfs_node->repo = fs_repo;
	ipfs_node->routing = ipfs_routing_new_kademlia(ipfs_node, &fs_repo->config->identity->private_key, stream);

	if (ipfs_node->routing == NULL)
		goto exit;

	//TODO ping kademlia

	retVal = 1;
	exit:
	if (ipfs_node->routing != NULL)
		stop_kademlia();
	return retVal;
}

int test_routing_supernode_get_value() {
	int retVal = 0;
	struct FSRepo* fs_repo = NULL;
	struct IpfsNode* ipfs_node = NULL;
	struct Stream* stream = NULL;
	int file_size = 1000;
	unsigned char bytes[file_size];
	char* fileName = "temp_file.bin";
	char* fullFileName = "/tmp/temp_file.bin";
	struct Node* write_node = NULL;
	size_t bytes_written = 0;
	unsigned char base58Hash[100];
	size_t results_size = 2048;
	char results_buffer[results_size];

	if (!drop_build_and_open_repo("/tmp/.ipfs", &fs_repo))
		goto exit;

	ipfs_node = (struct IpfsNode*)malloc(sizeof(struct IpfsNode));
	ipfs_node->mode = MODE_ONLINE;
	ipfs_node->identity = fs_repo->config->identity;
	ipfs_node->repo = fs_repo;
	ipfs_node->routing = ipfs_routing_new_kademlia(ipfs_node, &fs_repo->config->identity->private_key, stream);

	if (ipfs_node->routing == NULL)
		goto exit;

	// create a file
	create_bytes(&bytes[0], file_size);
	create_file(fullFileName, bytes, file_size);

	// write to ipfs
	if (ipfs_import_file("/tmp", fileName, &write_node, fs_repo, &bytes_written, 1) == 0) {
		goto exit;
	}

	if (!ipfs_node->routing->Provide(ipfs_node->routing, (char*)write_node->data, write_node->data_size))
		goto exit;
	// write_node->hash has the base32 key of the file. Convert this to a base58.
	if (!ipfs_cid_hash_to_base58(write_node->hash, write_node->hash_size, base58Hash, 100))
		goto exit;

	// ask the network who can provide this
	if (!ipfs_node->routing->FindProviders(ipfs_node->routing, (char*)base58Hash, 100, &results_buffer[0], &results_size))
		goto exit;

	// Q: What should FindProviders have in the results buffer? A: A struct of:
	// 20 byte id
	// 4 byte (or 16 byte for ip6) ip address
	// 2 byte port number
	// that means we have to attempt a connection and ask for peer ID
	// TODO: Q: How do we determine ip4 vs ip6?

	struct Libp2pLinkedList* multiaddress_head;
	// get an IP4 ip and port
	if (!ipfs_routing_supernode_parse_provider(&results_buffer, &multiaddress_head))
		goto exit;

	struct Libp2pLinkedList* current_address = multiaddress_head;
	struct MultiAddress* addr = NULL;
	while (current_address != NULL) {
		addr = (struct Multiaddress*)current_address->item;
		if (multiaddress_is_ip4(addr))
			break;
		addr = NULL;
		current_address = current_address->next;
	}

	if (addr == NULL)
		goto exit;

	// Connect to server
	char* ip;
	multiaddress_get_ip_address(addr, &ip);
	struct Stream* file_stream = libp2p_net_multistream_connect(ip, multiaddress_get_ip_port(addr));

	// Switch from multistream to NodeIO
	if (!libp2p_net_multistream_upgrade(file_stream, "/NodeIO/1.0.0"))
		goto exit;

	// Ask for file
	struct Node* node = libp2p_nodeio_get(file_stream, base58Hash, 100);
	if (node == NULL)
		goto exit;

	retVal = 1;
	exit:
	if (ipfs_node->routing != NULL)
		stop_kademlia();
	if (fs_repo != NULL)
		ipfs_repo_fsrepo_free(fs_repo);
	return retVal;

}
