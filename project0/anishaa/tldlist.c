/*
Author: Anisha Aggarwal
DuckID: anishaa
Title: CIS 415 Project 0
File: tldlist.c
Statement: Most of this work is mine.
	I had help from http://www.geeksforgeeks.org/avl-tree-set-1-insertion/
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tldlist.h"
#include "date.h"

struct tldlist {
	TLDNode *root;
	Date *begin;
	Date *end;
	long size;
	long itNodeCount;
};

struct tldnode{
	TLDNode *parent;
	TLDNode *left;
	TLDNode *right;
	long count;
	long height;
	char* content;
};

struct tlditerator{
	long next;
	long size;
	TLDNode **node_arr;
};

/*
* Helper Functions
*/

//gets the height of the node
static int height(TLDNode *node){
 	if(node == NULL){
 		return 0;
 	}
 	return node->height;
}

// balance the tree
static int getBalance(TLDNode *node){
	if(node == NULL){
		return 0;
	}

	return height(node->left) - height(node->right);
}

//gets the max between 2 integers
static int max(int x, int y){
 	if(x > y){
 		return x;
 	}else if(x < y){
 		return y;
 	}else{
 		return x;
 	}
}

//create a new node
TLDNode *newNode(char *tld) {
	TLDNode *node = (TLDNode *)malloc(sizeof(TLDNode));
	if (node == NULL) {
		return NULL;
	}

	node->left = NULL;
	node->right = NULL;
	node->count = 1;
	node->height = 1;
	node->content = tld;

	return node;
}

//preform a right rotation on AVL tree
static TLDNode *rightRotate(TLDNode *current) {
	TLDNode *node1 = current->left;
	TLDNode *node2 = node1->right;

	//perform rotation
	node1->right = current;
	current->left = node2;

	current->height = max(height(current->left), height(current->right)) + 1;
 	node1->height = max(height(node1->left), height(node1->right)) + 1;

	//new root
	return node1;
}

//preform a left rotation on AVL tree
static TLDNode *leftRotate(TLDNode *current) {
	TLDNode *node1 = current->right;
	TLDNode *node2 = node1->left;

	//perform rotation
	node1->left = current;
	current->right = node2;

	current->height = max(height(current->left), height(current->right)) + 1;
 	node1->height = max(height(node1->left), height(node1->right)) + 1;

	//new root
	return node1;
}

//insert function of a new node
static TLDNode *insert(TLDNode *tld, char* content, long *added_node) {
	if (tld == NULL) {
			*added_node = 1;
			return newNode(content);
	}

	if (strcmp(tld->content, content) < 0) {
		tld->left = insert(tld->left, content, added_node);
	} else if (strcmp(tld->content,content) > 0) {
		tld->right = insert(tld->right, content, added_node);
	} else {
		free(content);
		tld->count++;
		return tld;
	}

/*
	tld->height = 1 + max(height(tld->left), height(tld->right));

	int balance = getBalance(tld);

	if ((balance > 1) && (strcmp(content, tld->left->content) < 0)) {
		return rightRotate(tld);
	}

	if ((balance < -1) && (strcmp(content, tld->right->content) > 0)) {
		return leftRotate(tld);
	}

	if ((balance > 1) && (strcmp(content, tld->left->content) > 0)) {
		tld->left = leftRotate(tld->left);
		return rightRotate(tld);
	}

	if ((balance < -1) && (strcmp(content, tld->right->content) < 0)) {
		tld->right = rightRotate(tld->right);
		return leftRotate(tld);
	}
*/
	return tld;
}

//inorder traversal function
static void inOrder(TLDNode *node, TLDNode **node_arr, int *index){
	if(node != NULL){
		inOrder(node->left, node_arr, index);

		node_arr[*index] = node;
		(*index)++;

		inOrder(node->right, node_arr, index);
	}
}

/*
* Given functions to implement	
*/

/*
* tldlist_create generates a list structure for storing counts against
* top level domains (TLDs)
* creates a TLDList that is constrained to the `begin' and `end' Date's
* returns a pointer to the list if successful, NULL if not
*/
TLDList *tldlist_create(Date *begin, Date *end) {
	TLDList *list = (TLDList *)malloc(sizeof(TLDList));

	if (list != NULL) {
		list->begin = date_duplicate(begin);
		list->end = date_duplicate(end);
		list->root = NULL;
		list->size = 0;
		list->itNodeCount = 0L;
	}

	return list;
}

/*
* tldlist_destroy destroys the list structure in `tld'
* all heap allocated storage associated with the list is returned to the
* heap
*/
void tldlist_destroy(TLDList *tld) {
	//walk through the tree and delete memory allocated at each node
/*
	TLDIterator *tldit = tldlist_iter_create(tld);
	TLDNode *node;

	while((node = tldlist_iter_next(tldit)) != NULL) {
		free(node->content);
		free(node);
	}
*/
	date_destroy(tld->begin);
	date_destroy(tld->end);
	free(tld);
//	tldlist_iter_destroy(tldit);
}

/*
* tldlist_add adds the TLD contained in `hostname' to the tldlist if
* `d' falls in the begin and end dates associated with the list;
* returns 1 if the entry was counted, 0 if not
*/
int tldlist_add(TLDList *tld, char *hostname, Date *d) {
	
	char* content;	//content of TLD
	long added_node = 0L;	//checks if node was added by insert funtion

	//check if the date falls outside begin and end of TLD
	if ((date_compare(tld->begin, d) > 0) || (date_compare(d, tld->end) > 0)) {
		//printf("%s\n", "out of bounds");
		return 0;
	}

	//build the tld name
	char* h = strrchr(hostname, '.');
	if (h != NULL) {
		content = (char*)malloc((strlen(h) + 1));
		strcpy(content, h+1);
	} else {
		return 0;
	}

	//printf("%s\n", content);
	//insert this tld into the tldlist
	//TLDNode *n = newNode(content);
	tld->root = insert(tld->root, content, &added_node);
	if (added_node == 1L) {
		tld->itNodeCount++;
	}
	tld->size++;

	return 1;
}

/*
* tldlist_count returns the number of successful tldlist_add() calls since
* the creation of the TLDList
*/
long tldlist_count(TLDList *tld) {
	return tld->size;
}

/*
* tldlist_iter_create creates an iterator over the TLDList
* returns a pointer to the iterator if successful, NULL if not
*/
TLDIterator *tldlist_iter_create(TLDList *tld) {
	TLDIterator *tldit = (TLDIterator *)malloc(sizeof(TLDIterator));
	
	TLDNode **node_arr = (TLDNode **)malloc(tld->size * sizeof(TLDNode *));
	int index;
	index = 0;
	inOrder(tld->root, node_arr, &index);

	//check if memory was allocated
	if (tldit != NULL) {
		tldit->next = 0L;
		tldit->size = tld->itNodeCount;
		tldit->node_arr = node_arr;

	}

	return tldit;
}

/*
* tldlist_iter_next returns the next element in the list; returns a pointer
* to the TLDNode if successful, NULL if no more elements to return
*/
TLDNode *tldlist_iter_next(TLDIterator *iter) {
	TLDNode *next;
	if (iter != NULL) {
		if (iter->next < iter->size) {
			next = iter->node_arr[iter->next++];
			return next;
		} else {
			return NULL;
		}
	}
	return NULL;
}

/*
* tldlist_iter_destroy destroys the iterator specified by `iter'
*/
void tldlist_iter_destroy(TLDIterator *iter) {
	int i = 0;
	while(i < iter->size) {
		free(iter->node_arr[i++]);
	}

	free(iter->node_arr);
	free(iter);
}

/*
* tldnode_tldname returns the tld associated with the TLDNode
*/
char *tldnode_tldname(TLDNode *node) {
	return node->content;
}

/*
* tldnode_count returns the number of times that a log entry for the
* corresponding tld was added to the list
*/
long tldnode_count(TLDNode *node) {
	return node->count;
}



