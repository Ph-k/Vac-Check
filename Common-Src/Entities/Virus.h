#pragma once

typedef struct SkipList SkipList;
typedef struct BloomFilter BloomFilter;

// The data each virus has, does not need to be private to the implimantion files (as in the data structures)
typedef struct Virus {// Each virus has...
	SkipList *vaccinated_persons; // ...A vaccinated skip list...
	SkipList *not_vaccinated_persons; // ...A non-vaccinated skip list...
	BloomFilter *bloomFilter; // ...A bloom filter for the vaccinated...
	char* name; // ...And the (UNIQUE) name of the virus
} Virus;

// Creates a virus and its data structures
Virus* createVirus(char* name,int skipSize,int bloomSize);

// The virus strucure is latter saved in a hash table, thus wrappers are needed
int virusCmp(void *v1, void* v2);
void* GetVirusName(void* v);
unsigned int virusHash(void* key,unsigned int size);
void printVirus(void* v);
void deleteVirus(void* v);// Dealocates the memory of all the structures and the memory used for the virus