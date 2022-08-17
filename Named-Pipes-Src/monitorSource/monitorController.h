typedef struct Communicator Communicator;

int initilizeMonitor(int port,int numThreads,int cyclicBufferCount, unsigned int bloomSize, char** countryArgs, unsigned int socketBufferSize, Communicator **mainCommunicatorPointer);
int silentInsert(char* Id, char* lastName, char* firstName, char* country,unsigned int age, char* virusName,char vaccinated,char* date, pthread_mutex_t *mutexes);
int getCommandFromParrent(char* inputBuffer,unsigned int *maxInputStingSize);
int travelRequest(char* citizenID,char* virusName, char* requestedDate);
int addVaccinationRecords(char* country, int numThreads, int cyclicBufferCount);
int searchVaccinationStatus(char* citizenID);
void createLogFile();
int terminateMonitor();