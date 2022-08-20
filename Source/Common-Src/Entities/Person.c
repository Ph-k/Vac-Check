#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Person.h"
#include "StringDict.h"
#include "Communicator.h"

// Creates a new person
// (the countries dict is used in order to avoid having the same information saved more than once in the program)
Person* createPerson(char* id, char* firstName, char* lastName, char* country,unsigned int age,hashTable *countriesDict){

	Person* newPerson = malloc(sizeof(Person)); //Allocating memory for Person struct

	newPerson->id = malloc( (strlen(id)+1)*sizeof(char) ); //Allocating memory for id string, according to the given string size
	if(newPerson->id==NULL) return NULL; //Memory allocation failed
	strcpy(newPerson->id,id); //Copying string value

	newPerson->firstName = malloc( (strlen(firstName)+1)*sizeof(char) ); //Same as above
	if(newPerson->firstName==NULL) return NULL; //Memory allocation failed
	strcpy(newPerson->firstName,firstName);

	newPerson->lastName = malloc( (strlen(lastName)+1)*sizeof(char) ); //Same as above
	if(newPerson->lastName==NULL) return NULL; //Memory allocation failed
	strcpy(newPerson->lastName,lastName);

	char *countryFromDict = searchString(countriesDict,country);// Searching if the country has already be saved in the memory
	if(countryFromDict!=NULL){// If it is so
		// Duplicate string values are avoided, since the country of this persons points to the already allocated country string
		newPerson->country=countryFromDict;
	}else{// If the country string has not been allocated
		// The string is created
		newPerson->country = malloc( (strlen(country)+1)*sizeof(char) );
		if(newPerson->country==NULL) return NULL;
		strcpy(newPerson->country,country);
		
		//And saved in the hashtable for latter use
		if(insertString(countriesDict,newPerson->country)!=0) return NULL;
	}

	//Setting age integer value
	newPerson->age = age;
		
	return newPerson; //Returning person struct
}

// Given a person, and the data that makes up a person,
// The function compares the given the data with that of the person (in other words, all the data except the id)
int FullPersonCmp(Person *p1, char* firstName, char* lastName, char* country,unsigned int age){
	if(strcmp(p1->firstName,firstName)!=0) return -1;
	if(strcmp(p1->lastName,lastName)!=0) return -2;
	if(strcmp(p1->country,country)!=0) return -3;
	if(p1->age!=age) return -4;
	return 0;
}

// HashTable wrappers for person

// The "key" value of the person is his id
void* GetPersonId(void* p){
	return ((Person*)p)->id;
}

//This function compares two IDs (strings) in void* form
int PersonCmp(void *id1, void* id2){
	return strcmp(((const char*)id1),((const char*)id2));
}

// Hash function of person for hash table
unsigned int PersonHash(void* key,unsigned int size){
	char *k = (char*)key;
	unsigned int i, hash = 5381; //The hash is initialized as the lenght of the key logigal or with a prime number

	for (i=0; i<strlen(k); i++){
		//Each character in the key string characters are used along with some logical operations in order to create the hash
		hash ^= k[i] * 31;
		hash ^= hash << 7;
	}

	return hash%size; //(hash)mod(size) is used to not superpass the size of the hash table
}

// Prints a persons data to terminal
void printPerson(void* p){
	if(p == NULL) return;
	Person *person= (Person*)p;
	printf("%s %s %s %s %d\n",
		person->id,person->firstName,person->lastName,person->country,person->age);
}

int sendPerson(Person *p, Communicator *c){
	int ret;
	ret = sendMessage(c,p->id,strlen(p->id)*sizeof(char));
	if(ret != 0 ) return ret;
	ret = sendMessage(c,p->firstName,strlen(p->firstName)*sizeof(char));
	if(ret != 0 ) return ret;
	ret = sendMessage(c,p->lastName,strlen(p->lastName)*sizeof(char));
	if(ret != 0 ) return ret;
	ret = sendMessage(c,p->country,strlen(p->country)*sizeof(char));
	if(ret != 0 ) return ret;
	ret = sendMessage(c,&(p->age),sizeof(unsigned int));
	if(ret != 0 ) return ret;
	return 0;
}

int recievePerson(Communicator *c, char* id, char* firstName, char* lastName, char* country, unsigned int *age){
	int ret;
	unsigned size=100*sizeof(char);
	ret = recieveMessage(c,id,&size);
	if(ret != 0 ) return ret;
	size=100*sizeof(char);
	ret = recieveMessage(c,firstName,&size);
	if(ret != 0 ) return ret;
	size=100*sizeof(char);
	ret = recieveMessage(c,lastName,&size);
	if(ret != 0 ) return ret;
	size=100*sizeof(char);
	ret = recieveMessage(c,country,&size);
	if(ret != 0 ) return ret;
	size = sizeof(unsigned int);
	ret = recieveMessage(c,age,&size);
	if(ret != 0 ) return ret;
	return 0;
}

// Free all the memory that the given person has allocated
void deleteVoidPerson(void* p){
	Person *person= (Person*)p;
	if( person == NULL) return; //If no person was given, there is nothing to free
	free(person->id);
	free(person->firstName);
	free(person->lastName);
	//The country string is saved on the countries dictonary which is responsible of freeing the countries strings
	//free(person->country);
	free(person);
}