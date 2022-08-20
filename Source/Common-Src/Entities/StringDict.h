#pragma once
typedef struct hashTable hashTable;

// The string dictonary is exactly that, it wraps the hash table structure to a dictonary of strings

// Creating a dictonary
hashTable* initializeStringDict(int size);

// Inserting a string to the dictonary
int insertString(hashTable* dict,char* string);

// Searches for a string in the dictonary
char* searchString(hashTable* dict,char* string);

// Deleting a dictonary along with its strings
int destroyStringDict(hashTable* dict);