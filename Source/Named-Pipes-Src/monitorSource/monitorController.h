typedef struct Communicator Communicator;

int initilizeMonitor(char* commuicatorFile, Communicator **mainCommunicatorPointer);
int silentInsert(char* Id, char* lastName, char* firstName, char* country,unsigned int age, char* virusName,char vaccinated,char* date);
int getCommandFromParrent(char* inputBuffer,unsigned int *maxInputStingSize);
int travelRequest(char* citizenID,char* virusName, char* requestedDate);
int addVaccinationRecords(char* country);
int searchVaccinationStatus(char* citizenID);
void createLogFile();
int terminateMonitor();