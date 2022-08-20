#pragma once

// Some utility functions which are useful throughout the program

/*Given an unisgned int (ussualy 4 bytes long) it places it's bits to an 
arrey of chars (each char is ussualy 1 byte long, the smallest variable available)*/
unsigned char* unsignedIntToChars(unsigned int num);

/*Given an char arrey of unsigned int size, which will contain the bits of an unsigned int,
it "converts it" to an unsigned int variable*/
unsigned int charsToUnsignedInt(unsigned char* numArrey);

//Given an unsigned int it returns the number of digits in it's decimal represetation
int digitsCount(unsigned int num);

// Converts the given int to a string, taking care of all the needed memory allocations
char* strIntDup(int val);

// Converts the given unsigned int to a string, taking care of all the needed memory allocations
char* strUIntDup(unsigned int val);

// Given a buffer and it's size, it checks if all its bytes are set to '0'
int checkIfNull(char *buff, int buffsize);

// Hashes the key to an integre value, suposing it as a string
unsigned int voidStringHash(void* key,unsigned int size);

// Concatenates string1 and string2 to a new, taking care of all the necessary memory allocations
void myStringCat(char **concated,const char *string1,const char *string2);

// Checks if a string is a valid date
int checkIfValidDate(const char* dateString);

// Removes the all the c characters from the string, shifting all the others
void removeCharFromString(char* string,char c);

// Checks if a string is emty by means of only consisting of whitespaces
int checkEmptyString(const char* string);

// Compares two dates which are in string form
int dateCmp(char* Date_1, char* Date_2);

int dateDiffernceInMonths(char* Date_1, char* Date_2);

// Creates a new string which has the current date in it
char* getCurrentDate();