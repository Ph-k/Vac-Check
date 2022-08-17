#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h> // Needed for the max value of unsigned int
#include "HashTable.h"

typedef struct hashNode{ // Value of each cell of the hashtable
	void* item; //Item
	struct hashNode* next; // Pointer to the next node (in case of collision, else NULL)
} hashNode;

typedef struct hashTable{ // HashTable sturcture
	struct hashNode** table; // The table it self
	unsigned int size,occupiedEntries; // The size of the table & the entries which are occupied
	char underDuplication; // The hashTable is extendable, thus some instances of it might be temporary while duplicating
	// And the needed functions (detailed explanation in the declaration of the newHashTable() at HashTable.h)
	int (*cmpfun)(void *, void *);
	void* (*extractKey)(void*);
	unsigned int (*hash)(void*,unsigned int);
	void (*deleteItem)(void *);
	void (*printItem)(void*);
} hashTable;

hashTable* newHashTable( // Explanation of the arguments in the declaration of the newHashTable() at HashTable.h
	unsigned int size,
	int (*cmpfun)(void *, void *),
	void* (*extractKey)(void*),
	unsigned int (*hash)(void*,unsigned int),
	void (*printItem)(void*),
	void (*deleteItem)(void *)
){

	unsigned int i; 

	hashTable* hashT = malloc(sizeof(hashTable)); // Allocating hashtable structure
	if(hashT==NULL) return NULL; // Error if the memory allocation was not successful 
	
	hashT->size = size; // Initializing hashtable size
	hashT->occupiedEntries = 0; // At the beginning, all the cells of the table are empty
	hashT->table = malloc(sizeof(hashNode*) * size); // Allocating table of (size) pointers to hashNodes
	if(hashT->table==NULL) return NULL; // Error if the memory allocation was not successful 
	hashT->underDuplication=0; // A newly created has table is not being duplicated at the moment

	// Setting the pointers for the needed function of the item
	hashT->cmpfun=cmpfun;
	hashT->extractKey=extractKey;
	hashT->hash=hash;
	hashT->printItem=printItem;
	hashT->deleteItem=deleteItem;

	for(i=0; i<hashT->size; i++){
		hashT->table[i]=NULL; // Initializing the empty hash table
	}

	return hashT; // Returning the memory location of the hash table
}

int extendHashTable(hashTable *oldHashT){ // This function duplicates the size of a given hash table while preserving it's items
	if( oldHashT->size * 10 > UINT_MAX ) return 0; // Max size of the table is reached, no duplication can be done

	hashTable *newHashT = malloc(sizeof(hashTable)); // Allocating memory for a new hashtable
	memcpy(newHashT,oldHashT,sizeof(hashTable)); // Making a carbon of the given hashtable to the new, which will only have a different table
	newHashT->size = oldHashT->size * 10; // Initializing hashtable size (to the new size)
	newHashT->occupiedEntries = 0; // The new/carbon copy hashtable will have a different table, which will be empty at the beginning
	newHashT->table = malloc(sizeof(hashNode*) * newHashT->size); // Allocating the new (bigger) table
	newHashT->underDuplication=1; // This new hashtable is under duplication
	unsigned int i;
	for(i=0; i<newHashT->size; i++){
		newHashT->table[i]=NULL; //Initializing the new hash table as empty
	}

	void modifiedHashInsert(void* item,void *newHashT){ // Nested function which will fill the new hashtable
		if( HashInsert(newHashT, item) != 1) perror("Error in hash expansion");
	}
	// Traversing the old hashtable items and filling them to the new hashtable
	hashTraverse(oldHashT, &modifiedHashInsert, newHashT);

	hashNode *hashCell,*cellToBeDeleted;
	for(i=0; i<oldHashT->size; i++ ){ //For all the cells in old hash table
		hashCell = oldHashT->table[i]; 
		
		while( hashCell !=NULL ){ //And in collision which are in a list form
			// The memory is freed that concerns the hash table is freed,
			// but not the items since they are now a part of the new hashtable
			cellToBeDeleted = hashCell;
			hashCell = hashCell->next;
			free(cellToBeDeleted);
		}
	}
	free(oldHashT->table); //The memory of the old table of the hash table is freed

	// The hashtable now has ...
	oldHashT->size = newHashT->size; // ...A new bigger size...
	oldHashT->occupiedEntries = newHashT->occupiedEntries; // ...Another number of occupied entries
	oldHashT->table = newHashT->table; // ...A new bigger table
	free(newHashT); // freeing the tempory hash table structure
	return 0;
}

int HashInsert(hashTable* hashT, void* item){
	// If the hash table is almost full, more collisions will happen and performance will be affected...
	if( hashT->underDuplication!=1 && (float)hashT->occupiedEntries/(float)hashT->size > 0.6 ){
		if( extendHashTable(hashT)!=0 )// ...In order to avoid all that, we simply extend the hash table (only if it is not temporary)
			perror("Hash expansion failed!\n");
	}

	hashNode** hashCell = &(hashT->table[ hashT->hash(hashT->extractKey(item), hashT->size) ]); //& needed since the cell record might need to be modified

	if(*hashCell == NULL) (hashT->occupiedEntries)++; //If the cell will now be filled, we increase the number of occupied entries

	while(*hashCell != NULL){//If another record exist in this cell, the list of the cells is traversed untill the end

		//If the record exist in the hash table, it is not inserted
		if(hashT->cmpfun( hashT->extractKey((*hashCell)->item), hashT->extractKey(item) ) == 0) return -1;

		hashCell = &((*hashCell)->next);
	}

	if(*hashCell == NULL){
		(*hashCell) = malloc(sizeof(hashNode)); //Allocating memory for the new record
		(*hashCell)->item = item; //Initializing item data
		(*hashCell)->next = NULL;
		return 1; //Successfull insertion
	}
	
	return 0; // In any other case, something went wrong

}

void* hashFind(hashTable* hashT,void *key){
	hashNode* hashCell = hashT->table[ hashT->hash(key, hashT->size) ];
	
	while( hashCell!=NULL && hashT->cmpfun( hashT->extractKey(hashCell->item),key)!=0 ){//If there are collisions at this cell 
		hashCell = hashCell->next; //The record is searched in the list
	}
	
	return (hashCell != NULL) ? hashCell->item : NULL; //If the record was found, it's memory location is returned. Otherwise NULL
}

void hashPrint(hashTable* hashT){ //Function that prints the hash table and its data (for debugging purposes)
	hashNode* hashCell;

	printf("-----------------------------------\n");

	for(int i=0; i<hashT->size; i++ ){
		hashCell = hashT->table[i];
		printf("%d. ",i);
		while( hashCell !=NULL ){
			hashT->printItem(hashCell->item);
			printf("->");
			hashCell = hashCell->next;
		}
		
		//printf("NULL\n");
	}
	
	printf("-----------------------------------\n");
}

void hashTraverse(hashTable* hashT, void (*fun)(void* item,void *externalItem),void *externalItem){
	hashNode* hashCell;
	for(int i=0; i<hashT->size; i++ ){//Traversing all the cells of the table of the hash table
		hashCell = hashT->table[i];
		while( hashCell !=NULL ){//And calling the fun function for all the times of the cell
			fun(hashCell->item,externalItem);
			hashCell = hashCell->next;
		}
	}
}

int destroyHashTable(hashTable* hashT){
	hashNode *hashCell,*cellToBeDeleted;
	
	for(int i=0; i<hashT->size; i++ ){ //For all the cells in the hash table
		hashCell = hashT->table[i]; 
		
		while( hashCell !=NULL ){ //And each record that might be in a list form
			//All the momory is freed
			cellToBeDeleted = hashCell;
			hashT->deleteItem(cellToBeDeleted->item); //Along with the memory the item was occuping
			hashCell = hashCell->next;
			free(cellToBeDeleted);
		}
	}
	
	//In the end
	free(hashT->table); //The memory of the table of the hash table is freed
	free(hashT); //And the hashtable structure
	return 1;
}