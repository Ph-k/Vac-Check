/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
typedef struct bit_arrreyS bit_arrreyS; //Declaretion

// Creates the bit arrey structure
bit_arrreyS* createBitArrey(unsigned long int bits);

// Returns the value of the bit at the requested position
int getBitAt(bit_arrreyS* ba,unsigned long int position);

// Alters the value of the requested bit to value (value = 0 or 1)
int alterBitAt(bit_arrreyS* ba,unsigned long int position,int value);

// Simply prints the bit arrey (used for debugging purposes)
void printBitArrey(bit_arrreyS* ba);

// Frees all the memory the bit arrey strucure has allocated
void destroyBitArrey(bit_arrreyS* ba);


// These functions extend the utility of the bloom filter implimentation of the 1st assigment

// Getting the bit arrey itself, the arrey of chars (needed to send the arrey)
void getBitArreyArrey(bit_arrreyS* ba,const char **bitArrey, unsigned int *size);

// Concatenating the bit arrey with the given one (needed in the parrent process)
int BitArreyConcat(bit_arrreyS* ba,const char *bitArrey, unsigned int size);

// Creates a new bit arrey which will have the same values as the given one
bit_arrreyS* createBitArreyFromArrey(char* bitArrey,unsigned long int bits);