#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../DataStructures/BloomFilter.h"
#include "../DataStructures/SkipList.h"
#include "Person.h"
#include "Virus.h"

// Creates a virus and its data structures
Virus* createVirus(char* name,int skipSize,int bloomSize){

	Virus *newVirus = malloc(sizeof(Virus));
	if(newVirus==NULL) return NULL; //Memory allocation failed
	
	newVirus->name= malloc( (strlen(name)+1)*sizeof(char) ); //Allocating memory for name string
	if(newVirus->name==NULL) return NULL; //Memory allocation failed
	strcpy(newVirus->name,name); //Copying string value

	newVirus->vaccinated_persons = SkipListInitialize(skipSize,&PersonCmp);// Creating vaccinated skip list
	if(newVirus->vaccinated_persons==NULL) return NULL;// Creation went wrong
	
	newVirus->not_vaccinated_persons = SkipListInitialize(skipSize,&PersonCmp);// Creating non-vaccinated skip list
	if(newVirus->not_vaccinated_persons==NULL) return NULL;// Creation went wrong
	
	newVirus->bloomFilter = BloomFilterCreate(bloomSize);// Creating bloom filter
	if(newVirus->bloomFilter==NULL) return NULL;// Creation went wrong

	return newVirus; //Returning  virus struct
}


// The virus strucure is latter saved in a hash table, thus wrappers are needed

// Compares the names of two viruses
int virusCmp(void *v1, void* v2){
	return strcmp(((const char*)v1),((const char*)v2));
}

// The "key" of a virus is its name
void* GetVirusName(void* v){
	return ((Virus*)v)->name;
}

// Hashing function for the key-name of the virus
unsigned int virusHash(void* key,unsigned int size){ 
	char *k = (char*)key;
	unsigned int i, hash = 5381; //The hash is initialized as the lenght of the key logigal or with a prime number

	for (i=0; i<strlen(k); i++){
		//Each character in the key string characters are used along with some logical operations in order to create the hash
		hash ^= k[i] * 31;
		hash ^= hash << 7;
	}

	return hash%size; //(hash)mod(size) is used to not superpass the size of the hash table
}

// Printing a viruses data structures (not requested, for debugging purposes)
void printVirus(void* v){
	if(v == NULL) return;
	Virus *virus= (Virus*)v;
	
	printf("virus %s, data structures:\nvaccinated:",virus->name);
	SkipListPrint(virus->vaccinated_persons);
	printf("non-vaccinated:");
	SkipListPrint(virus->not_vaccinated_persons);
	
}

// Dealocates the memory of all the structures and the memory used for the virus
void deleteVirus(void* v){
	if(v == NULL) return;
	Virus *virus= (Virus*)v;

	SkipListDestroy(virus->vaccinated_persons);
	SkipListDestroy(virus->not_vaccinated_persons);
	BloomFilterDestroy(virus->bloomFilter);
	free(virus->name);
	free(virus);
}