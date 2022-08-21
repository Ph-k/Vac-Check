/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
#pragma once

typedef struct hashTable hashTable; // Declaretion of hash table structure
	
hashTable* newHashTable( // Function that creates the hashtable
	unsigned int size,
	// The hash table is generic, meaning that the items it saves are of void*, 
	// Thus some utility functions about the given items are needed in order for the hash table to function properly.
	int (*cmpfun)(void *, void *), // A function that compares the items
	void* (*extractKey)(void*), // A function that returns the key of the item (so it can be hashed)
	unsigned int (*hash)(void*,unsigned int), // A function that hashes a given key of an item
	void (*printItem)(void*), // A function that prints the item (used only for debugging purposes)
	void (*deleteItem)(void *) // A function that frees all the memory the given item was occuping
);

// Function inderting item to the hashtable
int HashInsert(hashTable* hashT, void* item);

// Function that frees all the memory the hashtable has allocated (along with items)
int destroyHashTable(hashTable* hashT);

// Seaching entry in the hashtable
void* hashFind(hashTable* hashT,void *key);
// The function bellow traverses the hash table and calls the fun function for each item it comes across
void hashTraverse(hashTable* hashT, void (*fun)(void* item,void *externalItem),void *externalItem);

// Function that prints the hash table and its data (for debugging purposes)
void hashPrint(hashTable* hashT);