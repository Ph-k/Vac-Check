typedef struct BloomFilter BloomFilter; // Declaretion of bloom fiter structure

// Function that creates a bloom filter of size bits
BloomFilter* BloomFilterCreate(unsigned long int size);

// Inserts a new record to the bloom filter using hashes
int BloomFilterInsert(BloomFilter* bf,unsigned char *str);

// Check if a value exests in the bloom filter using hashes
int BloomFilterCheck(BloomFilter* bf,unsigned char *str);

// Frees all the memory the structure occupied
void BloomFilterDestroy(BloomFilter* bf);

// These functions extend the utility of the bloom filter implimentation of the 1st assigment

// Getting the bit arrey of the bloom filter (needed to send the arrey)
void getBloomFilterBitArrey(BloomFilter* bf,const char **bitArrey, unsigned int *size);

// Concatenating the bit arrey of the bloom filter with the given one (needed int the parrent process)
int BloomFilterArreyConcat(BloomFilter* bf,const char *bitArrey, unsigned int size);

// Creates a new bloom filter which bit arrey will be identical to the given one
BloomFilter* BloomFilterCreateFromArrey(char* bitArrey,unsigned int size);