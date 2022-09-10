/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "HashTable.h"
#include "Utilities.h"
#include "Abacus.h"

// In order to count the request and access them in retavely fat way

// The requests in the last stage are grouped by virus,
typedef struct virusWabacus{
    char* virusName;
    Abacus *virusRequestsCounter;// and a counter for all the different vaccination dates
} virusWabacus;

// These functioin are wrapping the above structure to be fit in a HT

virusWabacus* newVirus(char* virusName, int HTSize){
    virusWabacus *c = malloc(sizeof(virusWabacus));
    c->virusName = strdup(virusName);
    c->virusRequestsCounter = initializeAbacus(HTSize);
    return c;
}

int virusWabacusCmp(void *c1, void *c2){
    return strcmp( (char*)c1, (char*)c2 );
}

void* extractVirusWabacusName(void* v){
    return ((virusWabacus*)v)->virusName;
}

void printVirusWabacusName(void* v){
    printf("%s ", ((virusWabacus*)v)->virusName );
}

void deleteVirusWabacus(void *v){
    virusWabacus *vir = (virusWabacus*)v;
    destroyAbacus(vir->virusRequestsCounter);
    free(vir->virusName);
    free(vir);
}

// The viruses then are grouped by country
typedef struct country{
    char* countryName;
    hashTable *virusRequests; // And each country has it's viruses counters
} country;

// The following again allow the above structure to be used in an HT

country* newCountry(char* countryName, int HTSize){
    country *c = malloc(sizeof(country));
    c->countryName = strdup(countryName);
    c->virusRequests = newHashTable(HTSize, virusWabacusCmp, extractVirusWabacusName, voidStringHash, printVirusWabacusName, deleteVirusWabacus);
    return c;
}

int countryCmp(void *c1, void *c2){
    return strcmp( (char*)c1, (char*)c2 );
}

void* extractCountry(void* c){
    return ((country*)c)->countryName;
}

void printCountry(void* c){
    printf("%s ", ((country*)c)->countryName );
}

void deleteCountry(void *c){
    country *coun = (country*)c;
    destroyHashTable(coun->virusRequests);
    free(coun->countryName);
    free(coun);
}

// And finay, a countries register for the travelStats has
typedef struct countriesRegister{
    hashTable *countries; // A has table to save all the different countries
    int HTSize;
} countriesRegister;

// Creation functio of a country request counter entity
countriesRegister* initilizeCountryRegister(int hashTableSize){
    countriesRegister *cr = malloc(sizeof(countriesRegister));
    cr->HTSize = hashTableSize;
    cr->countries = newHashTable(cr->HTSize, countryCmp, extractCountry, voidStringHash, printCountry, deleteCountry);
    return cr;
}

// Given the information of a travel request, it is counted in the correct abacus
int countRequest(countriesRegister *cr, char* countryName, char* virusName, char* date, char answer){
    // First we have to find the country, and create it if it does not exist
    country *coun = hashFind(cr->countries,countryName);
    if(coun == NULL) HashInsert(cr->countries, newCountry(countryName,cr->HTSize) );
    coun = hashFind(cr->countries,countryName);

    // The we find the virus structyre in the given country, again creating it if it does not exist
    virusWabacus* vir = hashFind(coun->virusRequests,virusName);
    if(vir == NULL) HashInsert(coun->virusRequests, newVirus(virusName,cr->HTSize) );
    vir = hashFind(coun->virusRequests,virusName);

    // Based on the awnser to the request (accepted/recected), we increse the right abacus counter
    switch (answer){
    case 'A':
        increaseColumn(vir->virusRequestsCounter,date,0);
        break;
    case 'R':
        increaseColumn(vir->virusRequestsCounter,date,1);
        break;
    default:
        return -1;
    }

    return 0;

}

// Given the arguments of a travel stats command, it calculates the awnser but summing the right requests
int getCounts(countriesRegister *cr, char *virusName, char *date1, char *date2, char *countryName,  unsigned int *accepted, unsigned int *rejected, unsigned int *total){
    // A dummy structure to pass arguments to the hash traversing function
    struct arguments{
        char *virusName;
        char *date1;
        char *date2;
        char *countryName;
        unsigned int *accepted;
        unsigned int *rejected;
        unsigned int *total;
    };

    // Saving arguments to structure
    struct arguments args;
    args.virusName = virusName;
    args.date1 = date1;
    args.date2 = date2;
    args.countryName = countryName;
    *accepted = 0;
    args.accepted = accepted;
    *rejected = 0;
    args.rejected = rejected;
    *total = 0;
    args.total = total;

    // This function traverses the countries HT and coutns only the requests for the given virus
    void countryValues(void *coun, void* vArgs){
        struct arguments *args = (struct arguments*)vArgs;

        // This funcion traverses the virus HT and counts only the requsests we want based on the date1, date2.
        void virusValues(void *vir, void* vArgs){
            unsigned int localTotal=0, localAccepted=0, localRejected=0;
            getColumnValuesIn(((virusWabacus*)vir)->virusRequestsCounter, date1, date2, &localAccepted, &localRejected, &localTotal);
            // Suming local requests
            *(args->accepted) = *(args->accepted) + localAccepted;
            *(args->rejected) = *(args->rejected)+ localRejected;
            *(args->total) = *(args->total) + localTotal;
        }

        // If a virus name has speciefied, we simply count its values
        if(virusName != NULL){
            virusWabacus* vir = hashFind(((country*)coun)->virusRequests,virusName);
            if(vir == NULL) {printf("no such virus\n"); return;}
            virusValues(vir,args);
        }else{
            // If no virus names was spacyfied we count everything for the given country
            hashTraverse(((country*)coun)->virusRequests,virusValues,&args);
        }
    }

    // If no country was specified
    if(countryName == NULL){
        // We count all the requests for all countries
        hashTraverse(cr->countries,countryValues,&args);
    }else{
        // If a country was specified, we only count its requests
        country *coun = hashFind(cr->countries,countryName);
        if(coun == NULL) return -1;
        countryValues(coun,&args);
    }
    return 0;
}

// Freeing all the memory this entity uses
void destroyCountryRegister(countriesRegister *cr){
    // By freeing the inital country HT, the hash table implimentation 
    // will call the free function of it's items and the items of their items etc.
    destroyHashTable(cr->countries);
    free(cr);
}