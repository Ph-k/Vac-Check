#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../DataStructures/HashTable.h"
#include "../Utilities/Utilities.h"

// The abacus structure consists only of a hashtable
typedef struct Abacus {
	hashTable* ht;
} Abacus;

// An abacus node is purpuse build for the needs of the travel command
typedef struct AbacusStruct {
	char *date; // It has the date of the request
	unsigned int accepted; // A accepted request counter
	unsigned int rejected; // A rejected request counter
} AbacusStruct;

// Wrapper functions in order to create a hashtable based on the abacus needs
int voidCmp(void* a,void* b){
	return strcmp((const char*)a, (const char*)b);
}

void* getAbacusName(void* as){
	return ((AbacusStruct*)as)->date;
}

void printAbacusColumn(void* as){
	printf("%s total: %d vac: %d ",(char*)(((AbacusStruct*)as)->date), (int)(((AbacusStruct*)as)->accepted), (int)(((AbacusStruct*)as)->rejected));
}

void deleteAbacusStruct(void* as){
	free( ((AbacusStruct*)as)->date );
	free(as);
}


// Implimentation

// Creates the an abacus (the size is the initial size of the hash table which is extendable)
Abacus* initializeAbacus(int size){
	Abacus* as = malloc(sizeof(Abacus));
	as->ht = newHashTable(size,&voidCmp,&getAbacusName,&voidStringHash,&printAbacusColumn,&deleteAbacusStruct);
	return as;
}

// Inserts a new abacus node, which has the aforementioned counters
int insertColumn(Abacus* abacus,char* date){
	AbacusStruct *as = malloc(sizeof(AbacusStruct));// Creating node
	as->accepted=0; // Initializing counters
	as->rejected=0;
	as->date=strdup(date); // The date string is duplicated since we do not know what the caller might do with the given string
	if( HashInsert(abacus->ht,as) !=1 ){// Finally the abacus node is inserted in the hashtable
		// Cleaning up the mess an insertion error creates
		free(as->date);
		free(as);
		return -1; // Returing insertion error
	}
	return 0;
}

// Given an abacus and the key (date) to one of its tuple counters, it increases the values of the according counters
int increaseColumn(Abacus* abacus,char* date,char mode){
	AbacusStruct *as = (AbacusStruct*)hashFind(abacus->ht,date);// First we need to find the abacus entry from the hash table
	if(as==NULL) { 
		// If the column does not exist, it is created
		insertColumn(abacus,date);
		as = (AbacusStruct*)hashFind(abacus->ht,date);
	}
	switch (mode){
		case 0:// Increase of mode 0 means that we only increase the accepted counter
			as->accepted += 1;
			break;
		case 1:// Increase of mode 1 means that we only increase the rejected counter
			as->rejected += 1;
			break;
		default: 
			return -1;
	}

	return 0;
}

// Given two dates it returns the value of the counters in the given date range
int getColumnValuesIn(Abacus* abacus, char* date1, char *date2, unsigned int *accepted, unsigned int *rejected, unsigned int *total){
	// A dummy struct to pass arguments to the hash traverse function
	struct result{
		unsigned int *accepted;
		unsigned int *rejected;
		char* date1;
		char* date2;
	};

	struct result res;
	*accepted = 0;
	res.accepted = accepted;
	*rejected = 0;
	res.rejected = rejected;
	res.date1 = date1;
	res.date2 = date2;

	// The dummy function to traverse the hashtable
	void countResult(void* abacusST, void *resultST){
		if ( date1==NULL ||
			( 
				dateCmp( ((AbacusStruct*)abacusST)->date , ((struct result*)resultST)->date1 ) >= 0 
				&& dateCmp( ((AbacusStruct*)abacusST)->date , ((struct result*)resultST)->date2 ) <= 0 
			)
		){
			// If a date was not specified or the current counter value is in the date range, we count the values of the counter
			*(((struct result*)resultST)->accepted) = *(((struct result*)resultST)->accepted) + ((AbacusStruct*)abacusST)->accepted;
			*(((struct result*)resultST)->rejected) = *(((struct result*)resultST)->rejected) + ((AbacusStruct*)abacusST)->rejected;
		}
	}

	hashTraverse(abacus->ht, countResult, &res); // Traversing the hashtable
	*total = *accepted + *rejected; // Calculating total value
	return 0;
}

// Frees all the memory allocated for the abacus structure
int destroyAbacus(Abacus* abacus){
	destroyHashTable(abacus->ht);
	free(abacus);
	return 0;
}