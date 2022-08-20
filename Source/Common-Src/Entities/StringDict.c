#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "HashTable.h"
#include "Utilities.h"


// Wrapper functions needed to use the hash table

int voidStrcmp(void* a,void* b){
	return strcmp((const char*)a,(const char*)b);
}

void *getVoidString(void* string){
	return string;
}

void printVoidString(void* string){
	printf("%s ",(char*)string);
}

// Implimentation
hashTable* initializeStringDict(int size){
	// A string dictonery is simply a hashtable with strings as items
	return newHashTable(size,&voidStrcmp,&getVoidString,&voidStringHash,&printVoidString,&free);
}

// Inserting a new string to the dictonery
int insertString(hashTable* dict,char* string){
	if( HashInsert(dict,string) !=1 ){ // In reallity the string is simply inserted to the hash table
		return -1;
	}
	return 0;
}

// Searching for a string to the dictonery
char* searchString(hashTable* dict,char* string){
	// In reallity the string is simply searched in the hash table
	return (char*)hashFind(dict,string);
}

// Deleting all the memory the dictonery and its items have allocated
int destroyStringDict(hashTable* dict){
	// Actually just deleting the hashtable
	return destroyHashTable(dict);
}