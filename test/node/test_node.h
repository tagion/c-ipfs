#include "ipfs/node/node.h"

int test_node() {
	//Variables of link:
	char * name = "Alex";
	unsigned char * ahash = "QmYwAPJzv5CZsnA625s3Xf2nemtYgPpHdWEz79ojWnPbdG";
	struct NodeLink * mylink;
	int retVal = ipfs_node_link_new(name,ahash, &mylink);
	printf("===================================\n"	\
			"Node Link:\n" 							\
		   " -Name: %s\n"   						\
		   "\n Cid Details:\n\n"					\
			   " -Version: %d\n"						\
		   " -Codec: %c\n"							\
			   " -Hash: %s\n"							\
			   " -Hash Length: %lu\n"  					\
			   "====================================\n" \
		   , mylink->name, mylink->cid->version,mylink->cid->codec,mylink->cid->hash,mylink->cid->hash_length);
	//Link Two for testing purposes
	char * name2 = "Simo";
	unsigned char * ahash2 = "QmYwAPJzv5CZsnA625s3Xf2nemtYgPpHdWEz79ojWnSimo";
	struct NodeLink * mylink2;
	retVal = ipfs_node_link_new(name2, ahash2, &mylink2);
	//Nodes
	struct Node * Mynode;
	retVal = ipfs_node_new_from_link(mylink, &Mynode);
	mylink->name = "HAHA";//Testing for valid node creation
	printf("Node Link[0] Name: %s\nHash: %s\n",Mynode->links[0]->name, Mynode->links[0]->cid->hash);
	Mynode = ipfs_node_add_link(&Mynode, mylink2, sizeof(mylink2));
	mylink2->name = "HAHA";//Testing for valid node creation
	printf("Node Link[1] Name: %s\nHash: %s\n",Mynode->links[1]->name,Mynode->links[1]->cid->hash);
	struct NodeLink * ResultLink = ipfs_node_get_link_by_name(Mynode, "Simo");
	printf("\nResultLink: \nName: %s\nHash: %s\n", ResultLink->name, ResultLink->cid->hash);
	ipfs_node_remove_link_by_name("Simo", Mynode);
	printf("Outlinkamt: %d\n", Mynode->link_amount);
	ipfs_node_link_free(mylink);
	ipfs_node_link_free(mylink2);
	ipfs_node_link_free(ResultLink);
	ipfs_node_free(Mynode);
	return 1;
}

int compare_link(struct NodeLink* link1, struct NodeLink* link2) {
	if (strcmp(link1->name, link2->name) != 0) {
		printf("Link Names are different %s vs. %s\n", link1->name, link2->name);
		return 0;
	}
	if (link1->cid->codec != link2->cid->codec) {
		printf("Link cid codecs are different. Expected %02x but got %02x\n", link1->cid->codec, link2->cid->codec);
		return 0;
	}
	if (link1->cid->hash_length != link2->cid->hash_length) {
		printf("Link cid hash lengths are different. Expected %d but got %d\n", link1->cid->hash_length, link2->cid->hash_length);
		return 0;
	}
	if (link1->cid->version != link2->cid->version) {
		printf("Link cid versions are different. Expected %d but got %d\n", link1->cid->version, link2->cid->version);
		return 0;
	}
	if (memcmp(link1->cid->hash, link2->cid->hash, link1->cid->hash_length) != 0) {
		printf("compare_link: The values of the hashes are different\n");
		return 0;
	}
	return 1;
}

int test_node_link_encode_decode() {
	struct NodeLink* control = NULL;
	struct NodeLink* results = NULL;
	size_t nl_size;
	unsigned char* buffer = NULL;
	int retVal = 0;

	// make a NodeLink
	if (ipfs_node_link_new("My Name", "QmMyHash", &control) == 0)
		goto exit;

	// encode it
	nl_size = ipfs_node_link_protobuf_encode_size(control);
	buffer = malloc(nl_size);
	if (buffer == NULL)
		goto exit;
	if (ipfs_node_link_protobuf_encode(control, buffer, nl_size, &nl_size) == 0) {
		goto exit;
	}

	// decode it
	if (ipfs_node_link_protobuf_decode(buffer, nl_size, &results) == 0) {
		goto exit;
	}

	// verify it
	if (compare_link(control, results) == 0)
		goto exit;
	retVal = 1;
exit:
	if (control != NULL)
		ipfs_node_link_free(control);
	if (results != NULL)
		ipfs_node_link_free(results);
	return retVal;
}

/***
 * Test a node with 2 links
 */
int test_node_encode_decode() {
	struct Node* control = NULL;
	struct Node* results = NULL;
	struct NodeLink* link1 = NULL;
	struct NodeLink* link2 = NULL;
	int retVal = 0;
	size_t buffer_length = 0;
	unsigned char* buffer = NULL;

	// node
	if (ipfs_node_new(&control) == 0)
		goto exit;

	// first link
	if (ipfs_node_link_new((char*)"Link1", (unsigned char*)"QmLink1", &link1) == 0)
		goto exit;

	if ( (control = ipfs_node_add_link(&control, link1, sizeof(link1))) == NULL)
		goto exit;

	// second link
	if (ipfs_node_link_new((char*)"Link2", (unsigned char*)"QmLink2", &link2) == 0)
		goto exit;
	if ( (control = ipfs_node_add_link(&control, link2, sizeof(link2))) == NULL)
		goto exit;

	// encode
	buffer_length = ipfs_node_protobuf_encode_size(control);
	buffer = (unsigned char*)malloc(buffer_length);
	if (ipfs_node_protobuf_encode(control, buffer, buffer_length, &buffer_length) == 0)
		goto exit;

	// decode
	if (ipfs_node_protobuf_decode(buffer, buffer_length, &results) == 0)
		goto exit;

	// compare results
	if (control->link_amount != results->link_amount || control->link_amount != 2)
		goto exit;

	for(int i = 0; i < control->link_amount; i++) {
		if (compare_link(control->links[i], results->links[i]) == 0) {
			printf("Error was on link %d\n", i);
			goto exit;
		}
	}

	if (control->data_size != results->data_size)
		goto exit;

	if (memcmp(results->data, control->data, control->data_size) != 0) {
		goto exit;
	}

	retVal = 1;
exit:
	// clean up
	if (control != NULL)
		ipfs_node_free(control);
	if (results != NULL)
		ipfs_node_free(results);
	if (link1 != NULL)
		ipfs_node_link_free(link1);
	if (link2 != NULL)
		ipfs_node_link_free(link2);
	if (buffer != NULL)
		free(buffer);

	return retVal;
}
