/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
#pragma once

typedef struct hashTable hashTable;// Forward declaration
typedef struct Communicator Communicator;

typedef struct Person {//Struct representing all the required information for a person
	char* id;
	char* firstName;
	char* lastName;
	char* country;
	unsigned int age;
} Person;

// Creates a new person
// (the countries dict is used in order to avoid having the same information saved more than once in the program)
Person* createPerson(char* id, char* firstName, char* lastName, char* country,unsigned int age,hashTable* countriesDict);

// Given a person, and the data that makes up a person,
// The function compares the given the data with that of the person (in other words, all the data except the id)
int FullPersonCmp(Person *p1, char* firstName, char* lastName, char* country,unsigned int age);

// Free all the memory that the given person has allocated
void deleteVoidPerson(void* p);

void* GetPersonId(void* p);
int PersonCmp(void *p1, void* p2);
unsigned int PersonHash(void* key,unsigned int size);
void printPerson(void* p);
void deleteVoidPerson(void* p);



// New functions for the needs of the 2nd assigment

// Send a person info using the communicator
int sendPerson(Person *p, Communicator *c);

// Recieve a person info using the communicator
int recievePerson(Communicator *c, char* id, char* firstName, char* lastName, char* country,unsigned int *age);