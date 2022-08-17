#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//The bit arrey consists of...
typedef struct bit_arrreyS {
   char *arrey; // An arrey of bytes (chars are 1 byte), since we cannot allocate an arrey of bits
   unsigned int num_of_chars; // The size of the above byte arrey
   unsigned long int num_of_bits; // The actual size of the bit-arrey in bits, might be smaller than the num_of_chars
} bit_arrreyS;

bit_arrreyS* createBitArrey(unsigned long int bits){
	if(bits<1) { perror("bit arrey needs to have at least one bit!\n"); return NULL;}

	// Calculating the size of the arrey needed to save (bits) bits
	unsigned int num_of_chars = bits/8/*Full needed bits*/ + (bits%8==0? 0 : 1)/*Remaining needed bits*/;

	bit_arrreyS *ba = malloc(sizeof(bit_arrreyS)); // Creating bit arrey structure
	char *arrey = malloc(sizeof(char)*num_of_chars);// Allocating memory the bit arrey (in bytes)
	
	if(arrey==NULL || ba==NULL) { perror("bit arrey memory allocation!\n"); return NULL;}

	// Initializing the whole array with 0
	for(unsigned int i=0; i<num_of_chars; i++){
		memset(&(arrey[i]),0,sizeof(char));
	}
	
	// Initializing the rest needed values for the bit array
	ba->num_of_bits=bits;
	ba->num_of_chars=num_of_chars;
	ba->arrey=arrey;
	
	return ba; // Returning the bit arrey structure
}

// Creates a new bit arrey which will have the same values as the given one
bit_arrreyS* createBitArreyFromArrey(char* bitArrey,unsigned long int bits){
	if(bits<1) { perror("bit arrey needs to have at least one bit!\n"); return NULL;}

	// Calculating the size of the arrey needed to save (bits) bits
	unsigned int num_of_chars = bits/8/*Full needed bits*/ + (bits%8==0? 0 : 1)/*Remaining needed bits*/;

	bit_arrreyS *ba = malloc(sizeof(bit_arrreyS)); // Creating bit arrey structure
	char *arrey = malloc(sizeof(char)*num_of_chars);// Allocating memory the bit arrey (in bytes)
	
	if(arrey==NULL || ba==NULL) { perror("bit arrey memory allocation!\n"); return NULL;}

	// Initializing the whole array with 0
	for(unsigned int i=0; i<num_of_chars; i++){
		memset(&(arrey[i]),0,sizeof(char));
	}

	// The only actual differnce from the createBitArrey
	// Finally copying the values of the given arrey to the newly created one
	memcpy(arrey,bitArrey,num_of_chars);
	
	// Initializing the rest needed values for the bit array
	ba->num_of_bits=bits;
	ba->num_of_chars=num_of_chars;
	ba->arrey=arrey;
	
	return ba; // Returning the bit arrey structure
}

// Concatenating the bit arrey with the given one (needed int the parrent process)
int BitArreyConcat(bit_arrreyS* ba,const char *bitArrey, unsigned int size){
	if(ba->num_of_chars != size) return -1; //We asume that num of bits is the same
	for(unsigned int i=0; i<size; i++){
		ba->arrey[i] |= bitArrey[i]; // The concatination can simly be done with OR
	}
	return 0;
}

// Getting the bit arrey itself, the arrey of chars (needed to send the arrey)
void getBitArreyArrey(bit_arrreyS* ba,const char **bitArrey, unsigned int *size){
	*bitArrey = ba->arrey;
	*size = ba->num_of_chars;
}

int getBitAt(bit_arrreyS* ba,unsigned long int position){
	if(position>ba->num_of_bits) return -1; // Error value returned if the requested position exceeds the size of the bit arrey

	unsigned long int char_index = position / 8, // Index of the byte (char) where the requested bit is in
					  bit_index = position % 8; // Offset of the requested bit in it's byte

	return (ba->arrey[char_index] >> bit_index) & 1; // Returning the value of that bit using logical opperations (shifing and masking)
}

int alterBitAt(bit_arrreyS* ba,unsigned long int position,int value){
	if(position>ba->num_of_bits || position<0) return -1; // Error value returned if the requested position exceeds the size of the bit arrey
	if(value!=1 && value!=0) return -2; // Error if the new value is not binary

	//Getting the position and the offset of the requested bit the same way as above (lines 39-40)
	unsigned long int char_index = position / 8,
					  bit_index = position % 8;
	
	// Altering the value of the bit
	if(value==1)
		ba->arrey[char_index] = ba->arrey[char_index] | (1 << bit_index); // Logical or with the 1 for 1
	else // (the bloom filter never requests a bit to be set to 0, but the bit arrey should offer that option)
		ba->arrey[char_index] = ba->arrey[char_index] & ~(1 << bit_index); // Logical and with 0 for 0
	
	return 0;
}

// This function simply prints the bit arrey on bit arrey format (was used for debugging purposes)
void printBitArrey(bit_arrreyS* ba){
	int j;
	unsigned long int total=0;
	for(unsigned long int i=0; i<ba->num_of_chars; i++){
		printf(" ");
		for(j=0; j<sizeof(char)*8; j++){
			printf( "%d",(ba->arrey[i] >> j ) & 1);
			total += (ba->arrey[i] >> j ) & 1;
		}
		printf(" ");
	}
	printf("total %ld\n",total);
}

void destroyBitArrey(bit_arrreyS* ba){
	if(ba==NULL) return; // If there is no structure to be freed there is nothing to be done
	free(ba->arrey);
	free(ba);
}

