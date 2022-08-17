#pragma once

/*An abacus is purpuse build structure for the needs of the travelstats command.
  It is a fast counter structure based on a hashtable,
  it can increase and find a requested counter values much faster than if we were using an arrey or list*/

typedef struct Abacus Abacus;

// Creates the an abacus (the size is the initial size of the hash table which is extendable)
Abacus* initializeAbacus(int size);

// Inserts a new abacus node, initializing the proper nodes
int insertColumn(Abacus* abacus,char* date);

// Given an abacus and the key (date), it increases the values of the according counters
int increaseColumn(Abacus* abacus,char* name,char mode);

void* getAbacusName(void* as);

// Given two dates it returns the value of the counters in the given date range
int getColumnValuesIn(Abacus* abacus, char* date1, char *date2, unsigned int *accepted, unsigned int *rejected, unsigned int *total);

// Frees all the memory allocated for the abacus structure
int destroyAbacus(Abacus* abacus);