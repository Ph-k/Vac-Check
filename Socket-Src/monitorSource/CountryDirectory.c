#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Entities/StringDict.h"

// A country directory has
typedef struct countryDirectory{
    char* countryName;// The name of the country directory
    hashTable* readCountryFiles;// The files of the directory that have been read
}countryDirectory;

// HT wrapper functions in order to impliment the countryDirectory entity/data strucure

// Creates a country along with an dictonary for file names of the directory
void* newCountryFileName(char* country){
    countryDirectory *cd = malloc(sizeof(countryDirectory));
    cd->countryName = strdup(country);
    cd->readCountryFiles =  initializeStringDict(10);
    return cd;
}

int cmpCountryDir(void* countryDir1, void* countryDir2){
    return strcmp(countryDir1,countryDir2);
}

void* getDirName(void* countryDir){
    return ((countryDirectory*)countryDir)->countryName;
}

void freeDirString(void* country){
    countryDirectory *cd = (countryDirectory*)country;
    free(cd->countryName);
    destroyStringDict(cd->readCountryFiles);
    free(cd);
}

void printCountryDir(void* countryDir){
    printf("%s ",(char*)(((countryDirectory*)countryDir)->countryName));
}

// Inserts one file name to the read files of the country
int insertFileToCountryDir(countryDirectory *cd, char* countryFile){
    return insertString(cd->readCountryFiles,strdup(countryFile));
}

// Checks if the given file of the directort has been read
int checkFileInCountryDir(countryDirectory *cd, char* countryFile){
    return ( searchString(cd->readCountryFiles,countryFile) == NULL )? 0 : 1 ;
}