#include <stdlib.h>
#include <stdio.h>
#include "BitArrey.h"
#include "HashFunctions.h"

// Number of hash functions used to hash the given string
#define K_HASH 16

//The bloom filter consists of...
typedef struct BloomFilter {
   bit_arrreyS *bit_arrrey; //...A bit arrey...
   unsigned long int size; // And the size of the bit arrey in bits
} BloomFilter;

BloomFilter* BloomFilterCreate(unsigned int size){
	BloomFilter* bf = malloc(sizeof(BloomFilter));// Allocating memory
	if(bf==NULL) { perror("bloom filter memory allocation!\n"); return NULL;}
	
	bf->size=size*8; // The given size is in bytes, whereas the size of the bit arrey will be in bits
	bf->bit_arrrey=createBitArrey(bf->size); // Creating the bit arrey
	if(bf->bit_arrrey==NULL) { perror("bloom filter bit-arrey memory allocation!\n"); return NULL;}
	
	return bf; // Returning the bloom filter data structure
}

int BloomFilterInsert(BloomFilter* bf, unsigned char *str){
	if(bf==NULL) return -2; // A bloom filter data structure must be given

	for(unsigned int i=0; i<K_HASH; i++){ // Using the value of all the requested hashing functions
		// We alter the respective bits (error value is somthing goes wrong)
		if( alterBitAt(bf->bit_arrrey, hash_i(str,i) % bf->size, 1) != 0) return -1;
	}

	return 0;
}

int BloomFilterCheck(BloomFilter* bf, unsigned char *str){
	if(bf==NULL) return -2; // A bloom filter data structure must be given
	for(unsigned int i=0; i<K_HASH; i++){ // Using the value of all the requested hashing functions
		// We return a false if a hash value in the bit arrey is equal to 0
		if( getBitAt(bf->bit_arrrey, hash_i(str,i) % bf->size) == 0) return 0;
	}

	return 1; // If all the hash values were equal to 1, then we ruturn true (perhaps false due to the nature of bloom filter)
}

// Creates a new bloom filter which bit arrey will be identical to the given one
BloomFilter* BloomFilterCreateFromArrey(char* bitArrey,unsigned int size){
	BloomFilter* bf = malloc(sizeof(BloomFilter));// Allocating memory
	if(bf==NULL) { perror("bloom filter memory allocation!\n"); return NULL;}
	
	bf->size=size*8; // The given size is in bytes, whereas the size of the bit arrey will be in bits
	bf->bit_arrrey=createBitArreyFromArrey(bitArrey,bf->size); // Creating the bit arrey based on the given one
	if(bf->bit_arrrey==NULL) { perror("bloom filter bit-arrey memory allocation!\n"); return NULL;}
	
	return bf; // Returning the bloom filter data structure
}

// Getting the bit arrey of the bloom filter (needed to send the arrey)
void getBloomFilterBitArrey(BloomFilter* bf,const char **bitArrey, unsigned int *size){
	getBitArreyArrey(bf->bit_arrrey,bitArrey,size);
}

// Concatenating the bit arrey of the bloom filter with the given one (needed int the parrent process)
int BloomFilterArreyConcat(BloomFilter* bf,const char *bitArrey, unsigned int size){
	return BitArreyConcat(bf->bit_arrrey,bitArrey,size);
}

void BloomFilterDestroy(BloomFilter* bf){
	if(bf==NULL) return; // If a bloom filter data structure is given, the memory it occupied will be freed
	destroyBitArrey(bf->bit_arrrey);
	free(bf);
}