/**
 * An implementation of an IPFS node
 * Copying the go-ipfs-node project
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inttypes.h"
#include "ipfs/cid/cid.h"

#include "ipfs/node/node.h"

// for protobuf Node                           data & data_size             encoded                   cid                       link_amount & links
enum WireType ipfs_node_message_fields[] = { WIRETYPE_LENGTH_DELIMITED, WIRETYPE_LENGTH_DELIMITED, WIRETYPE_LENGTH_DELIMITED, WIRETYPE_LENGTH_DELIMITED };
// for protobuf NodeLink                              name                     cid
enum WireType ipfs_node_link_message_fields[] = { WIRETYPE_LENGTH_DELIMITED, WIRETYPE_LENGTH_DELIMITED };

/*====================================================================================
 * Link Functions
 *===================================================================================*/

/* ipfs_node_link_new
 * @Param name: The name of the link (char *)
 * @Param size: Size of the link (size_t)
 * @Param ahash: An Qmhash
 */
int ipfs_node_link_new(char * name, unsigned char * ahash, size_t hash_size, struct NodeLink** node_link)
{
	*node_link = malloc(sizeof(struct NodeLink));
	if (*node_link == NULL)
		return 0;
	(*node_link)->name = malloc(strlen(name) + 1);
	if ( (*node_link)->name == NULL) {
		free(*node_link);
		return 0;
	}
	strcpy((*node_link)->name, name);
	(*node_link)->next = NULL;
	int ver = 0;
	if (ipfs_cid_new(ver, ahash, hash_size, CID_PROTOBUF, &(*node_link)->cid) == 0) {
		free(*node_link);
		return 0;
	}
	return 1;
}

/* ipfs_node_link_free
 * @param node_link: Free the link you have allocated.
 */
int ipfs_node_link_free(struct NodeLink * node_link)
{
	if (node_link != NULL) {
		if (node_link->cid != NULL)
			ipfs_cid_free(node_link->cid);
		if (node_link->name != NULL)
			free(node_link->name);
		free(node_link);
	}
	return 1;
}

size_t ipfs_node_link_protobuf_encode_size(struct NodeLink* link) {
	if (link == NULL)
		return 0;

	size_t size = 0;
	size += 11 + strlen(link->name);
	size += ipfs_cid_protobuf_encode_size(link->cid);
	return size;
}

int ipfs_node_link_protobuf_encode(struct NodeLink* link, unsigned char* buffer, size_t max_buffer_length, size_t* bytes_written) {
	size_t bytes_used = 0;
	int retVal = 0;
	*bytes_written = 0;
	retVal = protobuf_encode_length_delimited(1, ipfs_node_link_message_fields[0], link->name, strlen(link->name), &buffer[*bytes_written], max_buffer_length - *bytes_written, &bytes_used);
	if (retVal == 0)
		return 0;
	*bytes_written += bytes_used;
	// cid
	size_t cid_size = ipfs_cid_protobuf_encode_size(link->cid);
	unsigned char cid_buffer[cid_size];
	retVal = ipfs_cid_protobuf_encode(link->cid, cid_buffer, cid_size, &bytes_used);
	if (retVal == 0)
		return 0;
	retVal = protobuf_encode_length_delimited(2, ipfs_node_link_message_fields[1], (char*)&cid_buffer[0], bytes_used, &buffer[*bytes_written], max_buffer_length - *bytes_written, &bytes_used);
	if (retVal == 0)
		return 0;
	*bytes_written += bytes_used;
	return 1;
}

int ipfs_node_link_protobuf_decode(unsigned char* buffer, size_t buffer_length, struct NodeLink** link, size_t* bytes_read) {
	size_t pos = 0;
	int retVal = 0;
	*link = (struct NodeLink*)malloc(sizeof(struct NodeLink));
	(*link)->cid = NULL;
	(*link)->name = NULL;
	(*link)->next = NULL;
	unsigned char* temp_buffer = NULL;
	size_t temp_size;

	if (*link == NULL)
		goto exit;
	while(pos < buffer_length) {
		size_t bytes_read = 0;
		int field_no;
		enum WireType field_type;
		if (protobuf_decode_field_and_type(&buffer[pos], buffer_length, &field_no, &field_type, &bytes_read) == 0) {
			goto exit;
		}
		pos += bytes_read;
		switch(field_no) {
			case (1):
				if (protobuf_decode_string(&buffer[pos], buffer_length - pos, &((*link)->name), &bytes_read) == 0)
					goto exit;
				pos += bytes_read;
				break;
			case (2):
				if (protobuf_decode_length_delimited(&buffer[pos], buffer_length - pos, (char**)&temp_buffer, &temp_size, &bytes_read) == 0)
					goto exit;
				ipfs_cid_protobuf_decode(temp_buffer, temp_size, &((*link)->cid));
				pos += bytes_read;
				free(temp_buffer);
				temp_buffer = NULL;
				break;
		}
	}

	retVal = 1;

exit:
	if (retVal == 0) {
		if (link != NULL)
			ipfs_node_link_free(*link);
	}
	if (temp_buffer != NULL)
		free(temp_buffer);

	return retVal;
}

/***
 * return an approximate size of the encoded node
 */
size_t ipfs_node_protobuf_encode_size(struct Node* node) {
	size_t size = 0;
	// data
	size += 11 + node->data_size;
	// encoded
	size += 11;
	if (node->encoded != NULL)
		size += strlen((const char*)node->encoded);
	// cid (a.k.a. cached)
	size += 11 + ipfs_cid_protobuf_encode_size(node->cached);
	// links
	size += 11;
	struct NodeLink* current = node->head_link;
	while(current != NULL) {
		size += 11 + strlen(current->name) + ipfs_cid_protobuf_encode_size(current->cid);
		current = current->next;
	}
	return size;
}

/***
 * Encode a node into a protobuf byte stream
 * @param node the node to encode
 * @param buffer where to put it
 * @param max_buffer_length the length of buffer
 * @param bytes_written how much of buffer was used
 * @returns true(1) on success
 */
int ipfs_node_protobuf_encode(struct Node* node, unsigned char* buffer, size_t max_buffer_length, size_t* bytes_written) {
	// data & data_size
	size_t bytes_used = 0;
	*bytes_written = 0;
	int retVal = 0;
	retVal = protobuf_encode_length_delimited(1, ipfs_node_message_fields[0], (char*)node->data, node->data_size, &buffer[*bytes_written], max_buffer_length - *bytes_written, &bytes_used);
	if (retVal == 0)
		return 0;
	*bytes_written += bytes_used;
	int sz = 0;
	if (node->encoded != NULL)
		sz = strlen((char*)node->encoded);
	retVal = protobuf_encode_length_delimited(2, ipfs_node_message_fields[1], (char*)node->encoded, sz, &buffer[*bytes_written], max_buffer_length - *bytes_written, &bytes_used);
	if (retVal == 0)
		return 0;
	*bytes_written += bytes_used;
	// cid
	size_t cid_size = ipfs_cid_protobuf_encode_size(node->cached);
	unsigned char cid[cid_size];
	retVal = ipfs_cid_protobuf_encode(node->cached, cid, cid_size, &cid_size);
	if (retVal == 0)
		return 0;
	retVal = protobuf_encode_length_delimited(3, ipfs_node_message_fields[2], (char*)cid, cid_size, &buffer[*bytes_written], max_buffer_length - *bytes_written, &bytes_used);
	if (retVal == 0)
		return 0;
	*bytes_written += bytes_used;
	// links
	struct NodeLink* current = node->head_link;
	while(current != NULL) {
		// size + name + cid
		size_t link_buffer_size = 11 + ipfs_node_link_protobuf_encode_size(current);
		unsigned char link_buffer[link_buffer_size];
		retVal = ipfs_node_link_protobuf_encode(current, link_buffer, link_buffer_size, &link_buffer_size);
		if (retVal == 0)
			return 0;
		protobuf_encode_length_delimited(4, ipfs_node_message_fields[3], (char*)link_buffer, link_buffer_size, &buffer[*bytes_written], max_buffer_length - *bytes_written, &bytes_used);
		*bytes_written += bytes_used;
		current = current->next;
	}

	return 1;
}

/***
 * Decode a stream of bytes into a Node structure
 * @param buffer where to get the bytes from
 * @param buffer_length the length of buffer
 * @param node pointer to the Node to be created
 * @returns true(1) on success
 */
int ipfs_node_protobuf_decode(unsigned char* buffer, size_t buffer_length, struct Node** node) {
	/*
	 * Field 0: data
	 * Field 1: encoded
	 * Field 3: cid
	 * Field 4: links array
	 */
	size_t pos = 0;
	int retVal = 0;
	unsigned char* temp_buffer = NULL;
	size_t temp_size;
	struct NodeLink* temp_link = NULL;

	if (ipfs_node_new(node) == 0)
		goto exit;

	while(pos < buffer_length) {
		size_t bytes_read = 0;
		int field_no;
		enum WireType field_type;
		if (protobuf_decode_field_and_type(&buffer[pos], buffer_length, &field_no, &field_type, &bytes_read) == 0) {
			goto exit;
		}
		pos += bytes_read;
		switch(field_no) {
			case (1): // data
				if (protobuf_decode_length_delimited(&buffer[pos], buffer_length - pos, (char**)&((*node)->data), &((*node)->data_size), &bytes_read) == 0)
					goto exit;
				pos += bytes_read;
				break;
			case (2): // encoded
				if (protobuf_decode_length_delimited(&buffer[pos], buffer_length - pos, (char**)&((*node)->encoded), &temp_size, &bytes_read) == 0)
					goto exit;
				pos += bytes_read;
				break;
			case (3): // cid
				if (protobuf_decode_length_delimited(&buffer[pos], buffer_length - pos, (char**)&temp_buffer, &temp_size, &bytes_read) == 0)
					goto exit;
				pos += bytes_read;
				if (ipfs_cid_protobuf_decode(temp_buffer, temp_size, &((*node)->cached)) == 0)
					goto exit;
				free(temp_buffer);
				temp_buffer = NULL;
				break;
			case (4): // links
				if (protobuf_decode_length_delimited(&buffer[pos], buffer_length - pos, (char**)&temp_buffer, &temp_size, &bytes_read) == 0)
					goto exit;
				pos += bytes_read;
				if (ipfs_node_link_protobuf_decode(temp_buffer, temp_size, &temp_link, &bytes_read) == 0)
					goto exit;
				free(temp_buffer);
				temp_buffer = NULL;
				ipfs_node_add_link(*node, temp_link);
				break;
		}
	}

	retVal = 1;

exit:
	if (retVal == 0) {
		ipfs_node_free(*node);
	}
	if (temp_buffer != NULL)
		free(temp_buffer);

	return retVal;
}

/*====================================================================================
 * Node Functions
 *===================================================================================*/
/*ipfs_node_new
 * Creates an empty node, allocates the required memory
 * Returns a fresh new node with no data set in it.
 */
int ipfs_node_new(struct Node** node)
{
	*node = (struct Node *)malloc(sizeof(struct Node));
	if (*node == NULL)
		return 0;
	(*node)->cached = NULL;
	(*node)->data = NULL;
	(*node)->data_size = 0;
	(*node)->encoded = NULL;
	(*node)->head_link = NULL;
	return 1;
}

/**
 * Set the cached struct element
 * @param node the node to be modified
 * @param cid the Cid to be copied into the Node->cached element
 * @returns true(1) on success
 */
int ipfs_node_set_cached(struct Node* node, const struct Cid* cid)
{
	if (node->cached != NULL)
		ipfs_cid_free(node->cached);
	return ipfs_cid_new(cid->version, cid->hash, cid->hash_length, cid->codec, &(node->cached));
}

/*ipfs_node_set_data
 * Sets the data of a node
 * @param Node: The node which you want to set data in.
 * @param Data, the data you want to assign to the node
 * Sets pointers of encoded & cached to NULL /following go method
 * returns 1 on success 0 on failure
 */
int ipfs_node_set_data(struct Node * N, unsigned char * Data, size_t data_size)
{
	if(!N || !Data)
	{
		return 0;
	}
	N->encoded = NULL;
	N->cached = NULL;
	N->data = malloc(sizeof(unsigned char) * data_size);
	if (N->data == NULL)
		return 0;

	memcpy(N->data, Data, data_size);
	N->data_size = data_size;
	return 1;
}

/*ipfs_node_set_encoded
 * @param NODE: the node you wish to alter (struct Node *)
 * @param Data: The data you wish to set in encoded.(unsigned char *)
 * returns 1 on success 0 on failure
 */
int ipfs_node_set_encoded(struct Node * N, unsigned char * Data)
{
	if(!N || !Data)
	{
		return 0;
	}
	N->encoded = Data;
	//I don't know if these will be needed, enable them if you need them.
	//N->cached = NULL;
	//N->data = NULL;
	return 1;
}
/*ipfs_node_get_data
 * Gets data from a node
 * @param Node: = The node you want to get data from. (unsigned char *)
 * Returns data of node.
 */
unsigned char * ipfs_node_get_data(struct Node * N)
{
	unsigned char * DATA;
	DATA = N->data;
	return DATA;
}

struct NodeLink* ipfs_node_link_last(struct Node* node) {
	struct NodeLink* current = node->head_link;
	while(current != NULL) {
		if (current->next == NULL)
			break;
		current = current->next;
	}
	return current;
}

int ipfs_node_remove_link(struct Node* node, struct NodeLink* toRemove) {
	struct NodeLink* current = node->head_link;
	struct NodeLink* previous = NULL;
	while(current != NULL && current != toRemove) {
		previous = current;
		current = current->next;
	}
	if (current != NULL) {
		if (previous == NULL) {
			// we're trying to delete the head
			previous = current->next;
			ipfs_node_link_free(current);
			node->head_link = previous;
		} else {
			// we're in the middle or end
			previous = current->next;
			ipfs_node_link_free(current);
		}
		return 1;
	}
	return 0;
}

/*ipfs_node_free
 * Once you are finished using a node, always delete it using this.
 * It will take care of the links inside it.
 * @param N: the node you want to free. (struct Node *)
 */
int ipfs_node_free(struct Node * N)
{
	if(N != NULL)
	{
		// remove links
		struct NodeLink* current = N->head_link;
		while (current != NULL) {
			struct NodeLink* toDelete = current;
			current = current->next;
			ipfs_node_remove_link(N, toDelete);
		}
		if(N->cached)
		{
			ipfs_cid_free(N->cached);
		}
		if (N->data) {
			free(N->data);
		}
		if (N->encoded != NULL) {
			free(N->encoded);
		}
		free(N);
	}
	return 1;
}

/*ipfs_node_get_link_by_name
 * Returns a copy of the link with given name
 * @param Name: (char * name) searches for link with this name
 * Returns the link struct if it's found otherwise returns NULL
 */
struct NodeLink * ipfs_node_get_link_by_name(struct Node * N, char * Name)
{
	struct NodeLink* current = N->head_link;
	while(current != NULL && strcmp(Name, current->name) != 0) {
		current = current->next;
	}
	return current;
}

/*ipfs_node_remove_link_by_name
 * Removes a link from node if found by name.
 * @param name: Name of link (char * name)
 * returns 1 on success, 0 on failure.
 */
int ipfs_node_remove_link_by_name(char * Name, struct Node * mynode)
{
	struct NodeLink* current = mynode->head_link;
	struct NodeLink* previous = NULL;
	while( (current != NULL)
			&& (( Name == NULL && current->name != NULL )
			|| ( Name != NULL && current->name == NULL )
			|| ( Name != NULL && current->name != NULL && strcmp(Name, current->name) != 0) ) ) {
		previous = current;
		current = current->next;
	}
	if (current != NULL) {
		// we found it
		if (previous == NULL) {
			// we're first, use the next one (if there is one)
			if (current->next != NULL)
				mynode->head_link = current->next;
		} else {
			// we're somewhere in the middle, remove me from the list
			previous->next = current->next;
			ipfs_node_link_free(current);
		}

		return 1;
	}
	return 0;
}

/* ipfs_node_add_link
 * Adds a link to your nodse
 * @param mynode: &yournode
 * @param mylink: the CID you want to create a node from
 * @param linksz: sizeof(your cid here)
 * Returns your node with the newly added link
 */
int ipfs_node_add_link(struct Node* Nl, struct NodeLink * mylink)
{
	if(Nl->head_link != NULL) {
		// add to existing by finding last one
		struct NodeLink* current_end = Nl->head_link;
		while(current_end->next != NULL) {
			current_end = current_end->next;
		}
		// now we have the last one, add to it
		current_end->next = mylink;
	}
	else
	{
		Nl->head_link = mylink;
	}
	return 1;
}

/*ipfs_node_new_from_link
 * Create a node from a link
 * @param mylink: the link you want to create it from. (struct Cid *)
 * @param linksize: sizeof(the link in mylink) (size_T)
 * Returns a fresh new node with the link you specified. Has to be freed with Node_Free preferably.
 */
int ipfs_node_new_from_link(struct NodeLink * mylink, struct Node** node)
{
	*node = (struct Node *) malloc(sizeof(struct Node));
	if (*node == NULL)
		return 0;
	(*node)->head_link = NULL;
	ipfs_node_add_link(*node, mylink);
	(*node)->cached = NULL;
	(*node)->data = NULL;
	(*node)->data_size = 0;
	(*node)->encoded = NULL;
	return 1;
}

/**
 * create a new Node struct with data
 * @param data: bytes buffer you want to create the node from
 * @param data_size the size of the data buffer
 * @param node a pointer to the node to be created
 * returns a node with the data you inputted.
 */
int ipfs_node_new_from_data(unsigned char * data, size_t data_size, struct Node** node)
{
	if(data)
	{
		if (ipfs_node_new(node) == 0)
			return 0;
		return ipfs_node_set_data(*node, data, data_size);
	}
	return 0;
}

/***
 * create a Node struct from encoded data
 * @param data: encoded bytes buffer you want to create the node from. Note: this copies the pointer, not a memcpy
 * @param node a pointer to the node that will be created
 * @returns true(1) on success
 */
int ipfs_node_new_from_encoded(unsigned char * data, struct Node** node)
{
	if(data)
	{
		if (ipfs_node_new(node) == 0)
			return 0;
		(*node)->encoded = data;
		return 1;
	}
	return 0;
}
/*Node_Resolve_Max_Size
 * !!!This shouldn't concern you!
 *Gets the ammount of words that will be returned by Node_Resolve
 *@Param1: The string that will be processed (eg: char * sentence = "foo/bar/bin")
 *Returns either -1 if something went wrong or the ammount of words that would be processed.
*/
int Node_Resolve_Max_Size(char * input1)
{
	if(!input1)
	{
		return -1; // Input is null, therefor nothing can be processed.
	}
	char input[strlen(input1)];
	bzero(input, strlen(input1));
	strcpy(input, input1);
	int num = 0;
	char * tr;
	char * end;
	tr=strtok_r(input,"/",&end);
	for(int i = 0;tr;i++)
	{
		tr=strtok_r(NULL,"/",&end);
		num++;
	}
	return num;
}

/*Node_Resolve Basically stores everything in a pointer array eg: char * bla[Max_Words_]
 * !!!This shouldn't concern you!!!
 *@param1: Pointer array(char * foo[x], X=Whatever ammount there is. should be used with the helper function Node_Resolve_Max_Size)
 *@param2: Sentence to gather words/paths from (Eg: char * meh = "foo/bar/bin")
 *@Returns 1 or 0, 0 if something went wrong, 1 if everything went smoothly.
*/

int Node_Resolve(char ** result, char * input1)
{
	if(!input1)
	{
		return 0; // Input is null, therefor nothing can be processed.
	}
	char input[strlen(input1)];
	bzero(input, strlen(input1));
	strcpy(input, input1);
	char * tr;
	char * end;
	tr=strtok_r(input,"/",&end);
	for(int i = 0;tr;i++)
	{
		result[i] = (char *) malloc(strlen(tr)+1);
		strcpy(result[i], tr);
		tr=strtok_r(NULL,"/",&end);
	}
	return 1;
}

/*Node_Resolve_Links
 * Processes a path returning all links.
 * @param N: The node you want to get links from
 * @param path: The "foo/bar/bin" path
 */
struct Link_Proc * Node_Resolve_Links(struct Node * N, char * path)
{
	if(!N || !path)
	{
		return NULL;
	}
	int expected_link_ammount = Node_Resolve_Max_Size(path);
	struct Link_Proc * LProc = (struct Link_Proc *) malloc(sizeof(struct Link_Proc) + sizeof(struct NodeLink) * expected_link_ammount);
	LProc->ammount = 0;
	char * linknames[expected_link_ammount];
	Node_Resolve(linknames, path);
	for(int i=0;i<expected_link_ammount; i++)
	{
		struct NodeLink * proclink;
		proclink = ipfs_node_get_link_by_name(N, linknames[i]);
		if(proclink)
		{
			LProc->links[i] = (struct NodeLink *)malloc(sizeof(struct NodeLink));
			memcpy(LProc->links[i], proclink, sizeof(struct NodeLink));
			LProc->ammount++;
			free(proclink);
		}
	}
	//Freeing pointer array
	for(int i=0;i<expected_link_ammount; i++)
	{
		free(linknames[i]);
	}
	return LProc;
}
/*Free_link_Proc
 * frees the Link_Proc struct you created.
 * @param1: Link_Proc struct (struct Link_Proc *)
 */
void Free_Link_Proc(struct Link_Proc * LPRC)
{
	if(LPRC->ammount != 0)
	{
		for(int i=0;i<LPRC->ammount;i++)
		{
			ipfs_node_link_free(LPRC->links[i]);
		}
	}
	free(LPRC);
}

/*Node_Tree() Basically a unix-like ls
 *@Param1: Result char * foo[strlen(sentence)]
 *@Param2: char sentence[] = foo/bar/bin/whatever
 *Return: 0 if failure, 1 if success
*/
int Node_Tree(char * result, char * input1) //I don't know where you use this but here it is.
{
	if(!input1)
	{
		return 0;
	}
	char input[strlen(input1)];
	bzero(input, strlen(input1));
	strcpy(input, input1);
	for(int i=0; i<strlen(input); i++)
	{
		if(input[i] == '/')
		{
			input[i] = ' ';
		}
	}
	strcpy(result, input);
	return 1;
}
