/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
int initializeProgramMonitors(char* input_dir, int numMonitors, int bloomSize, unsigned int bufferSize,unsigned int cyclicBufferSize ,int numThreads);
int recieveBloomFilters(int numMonitors, int bloomSize);

int travelRequest(char* citizenID, char* date, char* countryFrom, char* countryTo, char* virusName);
int travelStats(char* virusName, char* date1, char* date2, char* countryName);
int addVaccinationRecords(char *country, char* input_dir, int numMonitors, int bloomSize);
int searchVaccinationStatus(char* citizenID, int numMonitors);
void printPids();

#define RESTFULLY 0
#define VIOLENTLY 1
void terminateTravelMonitor(int numMonitors, char method);