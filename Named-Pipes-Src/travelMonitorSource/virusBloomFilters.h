typedef struct virusBloomFilter virusBloomFilter;

// The travel monitor needs a way of organazing the bloom filters of all the viruses

// HT wrappers
virusBloomFilter* newVirusBloomFilter(char* virusName, char* bitArrey, unsigned int size);
int cmpVirusBloomFilterNames(void* name1, void* name2);
void* getVirusBloomFilterName(void* vbf);
void printVirusBloomFilterName(void* vbf);

typedef struct BloomFilter BloomFilter; // Declaretion of bloom fiter structure
BloomFilter* getVirusBloomFilter(virusBloomFilter* vbf);

// Given a virus bloom filter, it concatincates (logical OR) it's bit arrey with the given one
int VirusBloomFilterConcat(virusBloomFilter* vbf, const char *bitArrey, unsigned int size);

void deleteVirusBloomFilterName(void* VOIDvbf);