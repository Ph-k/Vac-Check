/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
typedef struct countriesRegister countriesRegister;

// This entity impliments the counter for the travelStats command
// By organizing counters of the abacus structs im hash tables of countries, viruses, dates

countriesRegister* initilizeCountryRegister(int hashTableSize);

// Given the info of a reqsuest, it is counted
int countRequest(countriesRegister *cr, char* countryName, char* virusName, char* date, char answer);

// Given the arguments of the travel stats, the result is calculated from the counter values
int getCounts(countriesRegister *cr, char *virusName, char *date1, char *date2, char *countryName,  unsigned int *accepted, unsigned int *rejected, unsigned int *total);

void destroyCountryRegister(countriesRegister *cr);
