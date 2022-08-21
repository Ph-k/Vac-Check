/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BloomFilter.h"

// The travel monitor needs a way of organazing the bloom filters of all the viruses
typedef struct virusBloomFilter{
    // This structure saves a virus name, and it's bloom filter
    BloomFilter *bf;
    char* name;
} virusBloomFilter;

// Wrappers so the above can be saved using the generic HT
virusBloomFilter* newVirusBloomFilter(char* virusName, char* bitArrey, unsigned int size){
    virusBloomFilter *vbf = malloc(sizeof(virusBloomFilter));
    vbf->name = strdup(virusName);
    vbf->bf = BloomFilterCreateFromArrey(bitArrey,size);
    return vbf;
}

int cmpVirusBloomFilterNames(void* name1, void* name2){
    return strcmp(name1,name2);
}

void* getVirusBloomFilterName(void* vbf){
    return ((virusBloomFilter*)vbf)->name;
}

// Returns the bloom filter of the given virus
BloomFilter* getVirusBloomFilter(virusBloomFilter* vbf){
    return vbf->bf;
}

// Concatenates the bloom filter of the given virus, with the given bloom filter
int VirusBloomFilterConcat(virusBloomFilter* vbf, const char *bitArrey, unsigned int size){
    return BloomFilterArreyConcat(vbf->bf,bitArrey,size);
}

// For debuging purpuses
void printVirusBloomFilterName(void* vbf){
    printf("bloom of %s here\n",((virusBloomFilter*)vbf)->name);
}

// Deletes the strucutere by freeing....
void deleteVirusBloomFilterName(void* VOIDvbf){
    virusBloomFilter* vbf = VOIDvbf;
    BloomFilterDestroy(vbf->bf);// The bloom filter
    free(vbf->name);// The string/name of the virus
    free(vbf);// The structure it self
}