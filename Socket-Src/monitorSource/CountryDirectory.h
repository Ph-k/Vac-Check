// Provides a structure to save all the files a country directory has
typedef struct countryDirectory countryDirectory;

// Wrapper functions to allow the structure to be saved on the generic hash table
void* newCountryFileName(char* country);
int cmpCountryDir(void* countryFile1, void* countryFile2);
void* getDirName(void* countryFile);
void freeDirString(void* countryFile);
void printCountryDir(void* countryFile);

// Inserts one file name to the read files of the country
int insertFileToCountryDir(countryDirectory *cd, char* countryFile);

// Checks if the given file of the directort has been read
int checkFileInCountryDir(countryDirectory *cd, char* countryFile);