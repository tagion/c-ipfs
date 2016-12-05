/**
 * An implementation of an IPFS node
 * Copying the go-ipfs-node project
 */
#ifndef IPFS_NODE_H
#define IPFS_NODE_H

#include "inttypes.h"
#include "ipfs/cid/cid.h"

/*====================================================================================
 *
 * Structures
 *
 *===================================================================================*/

struct Link
{
	char * name;
	size_t size;
	struct Cid * Lcid;
};

struct Node
{
	unsigned char * data;
	size_t data_size;
	unsigned char * encoded;
	struct Cid * cached;
	int link_ammount;
	struct Link * links[];
};

/*====================================================================================
 *
 * Functions
 *
 *===================================================================================*/

/*====================================================================================
 * Link Functions
 *===================================================================================*/
/* Create_Link
 * @Param name: The name of the link (char *)
 * @Param size: Size of the link (size_t)
 * @Param ahash: An Qmhash
 */
struct Link * Create_Link(char * name, unsigned char * ahash);

/* Free_Link
 * @param L: Free the link you have allocated.
 */
void Free_Link(struct Link * L);

/*====================================================================================
 * Node Functions
 *===================================================================================*/
/*Create_Empty_Node
 * Creates an empty node, allocates the required memory
 * Returns a fresh new node with no data set in it.
 */
struct Node * Create_Empty_Node();

/*Node_Set_Data
 * Sets the data of a node
 * @param Node: The node which you want to set data in.
 * @param Data, the data you want to assign to the node
 * returns 1 on success 0 on failure
 */
int Node_Set_Data(struct Node * N, unsigned char * Data);

/*Node_Get_Data
 * Gets data from a node
 * @param Node: = The node you want to get data from. (unsigned char *)
 * Returns data of node.
 */
unsigned char * Node_Get_Data(struct Node * N);

/**
 * set Cid on node
 * @param node the node
 * @param the Cid to use
 * @returns true(1) on success
 */
int Node_Set_Cid(struct Node* N, struct Cid* cid);

/*Node_Copy: Returns a copy of the node you input
 * @param Node: The node you want to copy (struct CP_Node *)
 * Returns a copy of the node you wanted to copy.
 */
struct Node * Node_Copy(struct Node * CP_Node);

/*Node_Delete
 * Once you are finished using a node, always delete it using this.
 * It will take care of the links inside it.
 * @param N: the node you want to free. (struct Node *)
 */
void Node_Delete(struct Node * N);

/*Node_Get_Link
 * Returns a copy of the link with given name
 * @param Name: (char * name) searches for link with this name
 * Returns the link struct if it's found otherwise returns NULL
 */
struct Link * Node_Get_Link(struct Node * N, char * Name);

/*Node_Remove_Link
 * Removes a link from node if found by name.
 * @param name: Name of link (char * name)
 * returns 1 on success, 0 on failure.
 */
int Node_Remove_Link(char * Name, struct Node * mynode);

/* N_Add_Link
 * Adds a link to your node
 * @param mynode: &yournode
 * @param mylink: the CID you want to create a node from
 * @param linksz: sizeof(your cid here)
 * Returns your node with the newly added link
 */
struct Node * N_Add_Link(struct Node ** mynode, struct Link * mylink, size_t linksz);

/*N_Create_From_Link
 * Create a node from a link
 * @param mylink: the link you want to create it from. (struct Cid *)
 * @param linksize: sizeof(the link in mylink) (size_T)
 * Returns a fresh new node with the link you specified. Has to be freed with Node_Free preferably.
 */
struct Node * N_Create_From_Link(struct Link * mylink);

/*N_Create_From_Data
 * @param data: bytes buffer you want to create the node from
 * returns a node with the data you inputted.
 */
struct Node * N_Create_From_Data(unsigned char * data);

/*Node_Resolve_Max_Size
 * !!!This shouldn't concern you!
 *Gets the ammount of words that will be returned by Node_Resolve
 *@Param1: The string that will be processed (eg: char * sentence = "foo/bar/bin")
 *Returns either -1 if something went wrong or the ammount of words that would be processed.
*/
int Node_Resolve_Max_Size(char * input1);

/*Node_Resolve Basically stores everything in a pointer array eg: char * bla[Max_Words_]
 * !!!This shouldn't concern you!!!
 *@param1: Pointer array(char * foo[x], X=Whatever ammount there is. should be used with the helper function Node_Resolve_Max_Size)
 *@param2: Sentence to gather words/paths from (Eg: char * meh = "foo/bar/bin")
 *@Returns 1 or 0, 0 if something went wrong, 1 if everything went smoothly.
*/
int Node_Resolve(char ** result, char * input1);

/**************************************************************************************************************************************
 *|||||||||||||||||||||||||||||||||||||||| !!!! IMPORTANT !!! ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*
 **************************************************************************************************************************************
 * Not sure where this is used, I'm making something to easen it up for all of you.
 * This in itself will get all the links for you in a link[] array inside Link_Proc
 * the memory allocation for storage will be noted in the ammount of links.
 * After being done with this, you have to free the array for which you will have a function specially made for you.
 *
 * What this does:
 * It searches for links using a path like /foo/bar/bin/, if links with those names are found in the node you specify, it stores them
 * in a custom struct, Link_Proc; which is what Node_Resolve_Link returns.
 * Notes:
 * Use it, free it, it's all already laid out for you.
 * There will also be a tutorial demonstrating it in the same folder here so everyone can understand this.
*/
struct Link_Proc
{
	char * remaining_links; // Not your concern.
	int ammount; //This will store the ammount of links, so you know what to process.
	struct Link * links[]; // Link array
};

/*Node_Resolve_Links
 * Processes a path returning all links.
 * @param N: The node you want to get links from
 * @param path: The "foo/bar/bin" path
 */
struct Link_Proc * Node_Resolve_Links(struct Node * N, char * path);

/*Free_link_Proc
 * frees the Link_Proc struct you created.
 * @param1: Link_Proc struct (struct Link_Proc *)
 */
void Free_Link_Proc(struct Link_Proc * LPRC);

/*Node_Tree() Basically a unix-like ls
 *@Param1: Result char * foo[strlen(sentence)]
 *@Param2: char sentence[] = foo/bar/bin/whatever
 *Return: 0 if failure, 1 if success
*/
int Node_Tree(char * result, char * input1); //I don't know where you use this but here it is.

#endif
