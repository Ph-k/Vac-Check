#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "Communicator.h"
#include "virusBloomFilters.h"
#include "travelController.h"
#include "countriesRegister.h"
#include "Person.h"
#include "StringDict.h"
#include "Utilities.h"
#include "LogFileWritter.h"
#include "HashTable.h"
#include "BloomFilter.h"

// The program default location of the fifo files
static const char FIFOS_DIR[] = "./Fifos/";

// This struct holds all the necessary information of one monitor proecess
typedef struct Monitor{
    Communicator* communicator; // A communicator which uses pipes for bi-directonal communication
    hashTable* countries; // A string dictonery which with all the countries the monitor manages
    pid_t pID; // It's process ID
} Monitor;

// Pointers to needed data structures (their scope is only in this file due to the static declaration)
static Monitor* monitorsArrey; // Arrey of all the information of the monitors
static hashTable* virusBloomFilters; // Hashtable which holds all the bloom filters
static countriesRegister* countriesAnswerRegister; // A counter structure used from the travelStats

// Program parameters (file scope)
static char* countryDataFilesDir; // Location of the input_dir directory
static int numOfMonitors, sizeOfBloom;
static unsigned int fifoBufferSize;

#define MAX_STRING_SIZE 100 // A max buffer for string size

// This function forks the parents process and maked the children execute a monitor process
// Which basicaly creates/initiates a monitor process
pid_t spawnMonitor(char* fifoPath){
    pid_t id = fork();
    if(id==0){
        // Child fork part, the adress spaces of the program is overwritten with a monitor process
        execl("./monitor","./monitor",fifoPath,NULL);
        // If the program counter gets here, exec went wrong
        perror("exec() failed!");
        exit(-2);
    }else if( id < 0 ){
        // If fork returned a negative number, something went wrong
        perror("fork() failed!");
        exit(-1);
    }
    // Otherwise a positive value was retunred by fork, which is the monitors pids.
    // So since we are executing the original travelMonitor and not the forked, we end the function and go on
    return id;
}

// This function initializes all needed data structures,
// and creates the monitor processes along with the needed communication pipes
int initializeProgramMonitors(char* input_dir, int numMonitors, int bloomSize, unsigned int bufferSize){
    int i;

    // Initializing data strcuture which is used for the travel request command
    countriesAnswerRegister = initilizeCountryRegister(10);

    // The length of the fifo path is the length of the fifo directory 
    // + a number (for ease we asume that it wont be bigger than the max int)
    char *fifoPath=malloc((digitsCount(INT_MAX)+strlen(FIFOS_DIR)+1)*sizeof(char));

    // Creating an arrey to save the information of all the monitors
    monitorsArrey = malloc(sizeof(Monitor)*numMonitors);

    // Saving arguments to local/file variables
    numOfMonitors = numMonitors;
    fifoBufferSize = bufferSize;
    sizeOfBloom = bloomSize;
    countryDataFilesDir = input_dir;

    // For all monitor procceses
    for(i=0; i<numMonitors; i++){

        //Creating a communication path (internaly it consists of 2 fifos)
        sprintf(fifoPath,"%s%d%c",FIFOS_DIR,i,'\0');
        if( createCommunicator(fifoPath) == -1 ) printf("Fifo creation for %d failed\n",i);

        //Creating monitors using fork and saving the monitors info
        monitorsArrey[i].pID = spawnMonitor(fifoPath); // Forking & exec
        monitorsArrey[i].communicator = openParrentCommunicator(fifoPath,bufferSize); // Open communicator (actually the fifo files)
        monitorsArrey[i].countries = initializeStringDict(1); // A dictonary which saves the countries of the monitor //make bigger
        sendMessage(monitorsArrey[i].communicator,&bloomSize,sizeof(int)); // Sending the bloomsize parameter to the monitor
    }
    free(fifoPath);

    // Allocating country directories to monitors happens here
    DIR *rootDir = opendir(input_dir);
    struct  dirent *direntp;
    char *recPath=NULL,*input_dirW=NULL;
    struct stat InodeInf;

    i=0;
    while ( (direntp=readdir(rootDir)) != NULL ){//While there are files/directories in the directory
        if(strcmp(direntp->d_name,".")==0 || strcmp(direntp->d_name,"..")==0) continue; //Skipping the entries '.' & '..'

        // The "full" path of the file/directory is created
        myStringCat(&input_dirW,input_dir,"/"); // ex Data + / = Data/
        myStringCat(&recPath,input_dirW, direntp->d_name); // ex Data/ + Greece = Data/Greece

        // If a non-folder file was found in the directory, there is nothing to do with it
        stat(recPath,&InodeInf);
        if((InodeInf.st_mode & S_IFMT) != S_IFDIR) printf("Error: %s is not a directory!\n",recPath);

        // The country folder is allocated to the i monitor
        insertString(monitorsArrey[i].countries,strdup(direntp->d_name)); // Saving that information
        sendMessage(monitorsArrey[i].communicator,recPath,sizeof(char)*strlen(recPath)); // Seding the path to the monitor

        // This is how we circularly allocate the country directories to the monitors
        i = (i+1)%(numMonitors);
    }
    if(recPath != NULL) free(recPath);
    if(input_dirW != NULL) free(input_dirW);

    // An EOF to all the monitor process means that we have send it all it's countries
    char e=EOF;
    for(i=0; i<numMonitors; i++){
        sendMessage(monitorsArrey[i].communicator,&e,sizeof(char));
    }
    closedir(rootDir);
    return 0;
}

// Given a monitor communicator it recievies one bloom filter from it
int recieveMonitorBloomFilters(Communicator *monitorCommunicator, int bloomSize,char* tempBitArrey){
    unsigned int size=MAX_STRING_SIZE;
    char* virusName=malloc(sizeof(char)*size);
    memset(virusName,'\0',size);
    virusBloomFilter* Tempvbf;

    // Recieving a virus name
    recieveMessage(monitorCommunicator,virusName,&size);

    if(virusName[0]!=EOF || size!=sizeof(char)){
        // If a virus name was recieved and not an end of sending was recievied
        size=bloomSize;
        memset(tempBitArrey,0,size);
        // We recieve the bloom filter of the virus
        recieveMessage(monitorCommunicator,tempBitArrey,&size);

        Tempvbf = hashFind(virusBloomFilters,virusName);
        if(Tempvbf != NULL){
            // If a bloom filter for the parrent exists, it is concated with the given one
            if ( VirusBloomFilterConcat(Tempvbf, tempBitArrey, bloomSize) != 0 ) printf("Bloom filter concat error\n");
        }else{
            // Otherwise a new bloom filter is created and saved
            HashInsert(virusBloomFilters,newVirusBloomFilter(virusName,tempBitArrey,size));
        };
        // End of function
        free(virusName);
        return 0;
    }

    // If an end of seding was recieved insted of a virus 1 is returned
    free(virusName);
    return 1;
}

// It recievies ALL bloom filters from ALL monitors (runs once, when the program initializes)
int recieveBloomFilters(int numMonitors, int bloomSize){
    //Initializing the arrey of pipe file descriptors for poll()
    struct pollfd *pfds=malloc(sizeof(struct pollfd)*numMonitors);
    int i;
    for(i=0; i<numMonitors; i++){
        pfds[i].fd = getReadFifoFd(monitorsArrey[i].communicator);
        pfds[i].events = POLLIN;//Saving the use of the pipe for poll()
    }

    // Creating a hash table which will save all the bloom filters of the monitors
    virusBloomFilters = newHashTable(1, cmpVirusBloomFilterNames, getVirusBloomFilterName, voidStringHash, printVirusBloomFilterName, deleteVirusBloomFilterName);
    int flag = numMonitors;
    char * tempBitArrey = malloc(sizeof(char)*bloomSize);
    while(flag!=0){//While there are still pipes open that might send messages
		if(poll(pfds, numMonitors, -1) == -1) {printf("poll() parrent\n"); return -3;}//Poll is used to "wait" until one or more pipes have data
		for(i=0; i<numMonitors; i++){//For all the pipes
			if(pfds[i].revents & POLLIN){//If according to poll() the pipe has data to be red
                // We recieve a bloom from the monitor (swich case expresion is executed only once, based on the C standard)
                switch (recieveMonitorBloomFilters(monitorsArrey[i].communicator,bloomSize,tempBitArrey)){
                    case 0:
                        break; // A bloom filter was recieved
                    case 1:
                        flag--; // An end message was recieved, meaning that this monitor sended all its bloom filters
                        break;
                    default:
                        perror("Bloom filter recieving error\n");
                        break;
                }
			}
		}
	}
    free(tempBitArrey);
    free(pfds);
    return 0;
}

// Given a monitor number, it creates a new one to take its place
int recreateMonitor(int monitorNum){
    int i = monitorNum;
    // Closing the fifo used from the dead monitor
    closeAndDestroyCommunicator(monitorsArrey[i].communicator);

    // Creating ONE new monitor as in the initializeProgramMonitors
    char *fifoPath=malloc((digitsCount(INT_MAX)+strlen(FIFOS_DIR)+1)*sizeof(char));
    sprintf(fifoPath,"%s%d%c",FIFOS_DIR,i,'\0');
    if( createCommunicator(fifoPath) == -1 ) printf("Fifo creation for %d failed\n",i);

    monitorsArrey[i].pID = spawnMonitor(fifoPath);
    monitorsArrey[i].communicator = openParrentCommunicator(fifoPath,fifoBufferSize);
    int bloomSize = sizeOfBloom;
    sendMessage(monitorsArrey[i].communicator,&bloomSize,sizeof(int));

    free(fifoPath);

    // Nested function to send countries
    void dummyCountriesSenter(void* countryName, void* voidi){
        int i = *((int*)voidi);
        char *input_dirW=NULL, *recPath=NULL, *input_dir=strdup(countryDataFilesDir);

        myStringCat(&input_dirW,input_dir,"/");//The path of the file/directory is created
        myStringCat(&recPath,input_dirW, (char*)countryName);//The path of the file/directory is created
        sendMessage(monitorsArrey[i].communicator,recPath,sizeof(char)*strlen(recPath));

        if(input_dirW!=NULL) free(input_dirW);
        if(recPath!=NULL) free(recPath);
        free(input_dir);
    }

    // Traversing the countries the dead monitor was responisble for, in order to send them to the new one
    hashTraverse(monitorsArrey[i].countries,dummyCountriesSenter,&i);

    // Sending end-of-countries message
    char e = EOF;
    sendMessage(monitorsArrey[i].communicator,&e,sizeof(char));


    // Recieving all the bloom filters of the monitor (they should be the same with the dead monitor)
    char * tempBitArrey = malloc(sizeof(char)*bloomSize);
    while( recieveMonitorBloomFilters(monitorsArrey[i].communicator, bloomSize,tempBitArrey) == 0) {};
    free(tempBitArrey);

    return 0;
}

// It checks which monitors have terminated unexpectidly, and re-creates them
int checkMonitorsExistance(){
    int i,ret=0;
    for(i=0; i<numOfMonitors; i++){
        // waitpid with this arguemnts does not "block", simply returens !=0 if the pids does not exist
        if ( waitpid(monitorsArrey[i].pID, NULL, WNOHANG)!=0 ){
            printf("Monitor number %d was tarminated unexpectedly, plase wait for it's re-creation ...",i);
            recreateMonitor(i);
            printf(" recreated!\n");
            ret++;
        }
    }
    return ret;
}

// It searches the arrey of monitors, and finds the monitor which is responible for the given country
Monitor* getMonitorResponsibleForCountry(char* country){
    for(int i=0; i<numOfMonitors; i++){
        if( searchString(monitorsArrey[i].countries,country) != NULL ){
            return &(monitorsArrey[i]);
        }
    }
    return NULL;
}

// Impliments the travel request command
int travelRequest(char* citizenID, char* date, char* countryFrom, char* countryTo, char* virusName){
    if( getMonitorResponsibleForCountry(countryTo) == NULL)
        printf("ERROR: '%s' NO SUCH COUNTRY!\n", countryTo );

    Monitor *MonitorResponsibleForCountry;
    MonitorResponsibleForCountry = getMonitorResponsibleForCountry(countryFrom);
    if( MonitorResponsibleForCountry == NULL)
        printf("ERROR: '%s' NO SUCH COUNTRY!\n", countryFrom );

    virusBloomFilter *vbf = hashFind(virusBloomFilters,virusName);
    if(vbf == NULL){ printf("ERROR: NO SUCH VIRUS!\n"); return -1; }

    char *answer;
    unsigned int size;

    // First checking the parrents bloom filter
    BloomFilter* bf = getVirusBloomFilter(vbf);
    if( BloomFilterCheck(bf,(unsigned char*)citizenID) == 0 ){
        printf("REQUEST REJECTED – YOU ARE NOT VACCINATED\n");
        countRequest(countriesAnswerRegister, countryFrom,virusName, date,'R');
    }else{
        // Requesting from the right monitor to awnser to our command
        sendMessage(MonitorResponsibleForCountry->communicator,"travelRequest",sizeof(char)*13);
        sendMessage(MonitorResponsibleForCountry->communicator,citizenID,sizeof(char)*strlen(citizenID));
        sendMessage(MonitorResponsibleForCountry->communicator,virusName,sizeof(char)*strlen(virusName));
        sendMessage(MonitorResponsibleForCountry->communicator,date,sizeof(char)*strlen(date));

        size=12; // The anwser from the bloom is a message of MAX 12
        answer = malloc(sizeof(char)*size);
        memset(answer,'\0',size);
        // Recieve anwser from monitor
        recieveMessage(MonitorResponsibleForCountry->communicator,answer,&size);
        if(strcmp("YES\0",answer)==0){
            // Counting request anwser with the appropriate collection of data structures for the travelStats
            printf("REQUEST ACCEPTED - HAPPY TRAVELS\n");
            countRequest(countriesAnswerRegister, countryFrom,virusName, date,'A');
        }else if(strcmp("NO\0",answer)==0){
            printf("REQUEST REJECTED - YOU ARE NOT VACCINATED\n");
            countRequest(countriesAnswerRegister, countryFrom,virusName, date,'R');
        }else if(strcmp("YES_BUT_NO\0",answer)==0){
            printf("REQUEST REJECTED - YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n");
            countRequest(countriesAnswerRegister, countryFrom,virusName, date,'R');
        }else printf("ERROR: DID NOT RECIEVE ANSWER CORRECTLY (%s)!\n",answer);
        free(answer);
    }

    return 0;
}

// Impliments the travel stats command
int travelStats(char* virusName, char* date1, char* date2, char* countryName){
    unsigned int total, accepted, rejected;
    // Getting counter values for the given arguments and printing them
    int ret = getCounts(countriesAnswerRegister, virusName, date1, date2,  countryName, &accepted, &rejected, &total);
    printf("TOTAL REQUESTS %d\nACCEPTED %d\nREJECTED %d\n",total, accepted, rejected);
    return ret;
}

// Impliments the add vaccination records command
int addVaccinationRecords(char *country, char* input_dir, int numMonitors, int bloomSize){
    Monitor *MonitorResponsibleForCountry = getMonitorResponsibleForCountry(country);
    if(MonitorResponsibleForCountry == NULL) { printf("ERROR: NO SUCH COUNTRY!\n"); return -1;}

    // Sending signal to child
    kill(MonitorResponsibleForCountry->pID,SIGUSR1);

    // Creating and checking the path of the directory the new file is in
    char *recPath=NULL,*input_dirW=NULL;
    struct stat InodeInf;
    myStringCat(&input_dirW,input_dir,"/");
    myStringCat(&recPath,input_dirW, country);
    stat(recPath,&InodeInf);
    if((InodeInf.st_mode & S_IFMT) != S_IFDIR) printf("Error: %s is not a directory!\n",recPath);

    // Waiting for monitor to send a dummy acknowledgement message after it gets ready and before sending the data
    unsigned int s=sizeof(char)*4;
    char *ackString = malloc(s);
    recieveMessage(MonitorResponsibleForCountry->communicator,ackString,&s);
    free(ackString);

    // Sending the directory with the new file
    sendMessage(MonitorResponsibleForCountry->communicator,recPath,sizeof(char)*strlen(recPath));
    if(recPath != NULL) free(recPath);
    if(input_dirW != NULL) free(input_dirW);

    // Recieving all the new bloom filters
    char * tempBitArrey = malloc(sizeof(char)*bloomSize);
    while( recieveMonitorBloomFilters(MonitorResponsibleForCountry->communicator, bloomSize,tempBitArrey) == 0) {};
    free(tempBitArrey);

    return 0;
}

// Impliments the search vaccination status command
int searchVaccinationStatus(char* citizenID, int numMonitors){
    int i;
    //Initializing the arrey of pipe file descriptors for poll()
    struct pollfd *pfds=malloc(sizeof(struct pollfd)*numMonitors);
    for(i=0; i<numMonitors; i++){

        // Also sending the command through the pipe to all the monitors
        sendMessage(monitorsArrey[i].communicator,"searchVaccinationStatus",sizeof(char)*24);
        sendMessage(monitorsArrey[i].communicator,citizenID,sizeof(char)*strlen(citizenID));

        pfds[i].fd = getReadFifoFd(monitorsArrey[i].communicator);
        pfds[i].events = POLLIN; //Saving the use of the pipe for poll()
    }

    int flag = numMonitors;
    unsigned int size;
    char *id=malloc(sizeof(char)*100),
    *firstName=malloc(sizeof(char)*100),
    *lastName=malloc(sizeof(char)*100),
    *country=malloc(sizeof(char)*100),
    *message=malloc(sizeof(char)*25),
    *virus=malloc(sizeof(char)*100),
    *date=malloc(sizeof(char)*11/*11 = strlen("12-12-9999 ")*/);
    unsigned int age=0;
    while(flag!=0){//While there are still pipes open that might send messages
		if(poll(pfds, numMonitors, -1) == -1) {printf("poll() parrent\n"); return -3;}//Poll is used to "wait" until one or more pipes have data
		for(i=0; i<numMonitors; i++){//For all the pipes
			if(pfds[i].revents & POLLIN){//If according to poll() the pipe has data to be red
                size = 25; // The response is max 25
                memset(message,'\0',size);
                recieveMessage(monitorsArrey[i].communicator,message,&size);
				if( strcmp("GotCitizenWithThisId",message) == 0 ){ // This monitor has this citizen
                    // Recieving citizen info and printing it
                    memset(id,'\0',100); memset(firstName,'\0',100); memset(lastName,'\0',100); memset(country,'\0',100);
					recievePerson(monitorsArrey[i].communicator, id, firstName, lastName, country, &age);
                    printf("%s %s %s %s\nAGE %d\n",id, firstName, lastName, country, age);

				}else if( strcmp("ForVirus",message) == 0 ){
                    // The monitor sends information about a vaccination for this person
                    size = MAX_STRING_SIZE;
                    memset(virus,'\0',size);
                    recieveMessage(monitorsArrey[i].communicator,virus,&size);

                    size = 11; // Date is a string with a max lenght of 11
                    memset(date,'\0',size);
                    recieveMessage(monitorsArrey[i].communicator,date,&size);
                    if( strcmp(date,"NO")==0 ){
                        printf("%s %sT YET VACCINATED\n",virus,date); // date is 'NO' so NOT is printed :)
                    }else{
                        printf("%s VACCINATED ON %s\n",virus,date);
                    }

                }else if( strcmp("NoCitizenWithThisId",message) == 0 || strcmp("GaveEvrythingThisId",message) == 0){
                    // If the monitor send evrything it had, or it does not have any info for this citizen, we degrease the flag
                    flag--;
                }
			}
		}
	}

    free(id); free(firstName); free(lastName); free(country); free(message); free(virus); free(date); free(pfds);
    return 0;
}

// Impliments an extra command "printPids" which simply shows the process ids of the program
void printPids(){
    printf("travelMonitor (parrent process) pid: %d\nmonitors (children):",getpid());
    for(int i=0; i<numOfMonitors; i++){
        printf(" pid of no%d=%d",i,monitorsArrey[i].pID);
        if(i!=numOfMonitors-1) putchar(',');
    }
    printf("\n");
}

// Terminates all programs, in two ways, always cleans the memory of the parrent, but sigkilling the monitors by default
void terminateTravelMonitor(int numMonitors, char method){
    int i;

    for(int i=0; i<numMonitors; i++){
        // Writing all the countries to the log file
        hashTraverse(monitorsArrey[i].countries, dummyCountyLogWritter, NULL);

        if(method == VIOLENTLY){
            // The deafult exit behavior, killing all the children with sigkill without giving them the option to free memory
            if ( kill(monitorsArrey[i].pID,9) == -1) perror("Could not kill a child: ");
        }else{
            // A non requested exit method, it tells the monitors to free their memory and terminate
            sendMessage(monitorsArrey[i].communicator,"exit\0",6*sizeof(char));
        }

        // Clossing communicator
        closeAndDestroyCommunicator(monitorsArrey[i].communicator);
        // Freeing memory of dictonary
        destroyStringDict(monitorsArrey[i].countries);
    }

    // Waiting for all monitors to terminate (since wait(NULL) waits for one children to terminate)
    for(i=0; i<numMonitors; i++){
        wait(NULL);
    }

    // Getting the total stats and writing them to the log file
    unsigned int total=0, accepted=0, rejected=0;
    getCounts(countriesAnswerRegister, NULL, NULL, NULL, NULL, &accepted, &rejected, &total);
    char* buffer = malloc(sizeof(char)*(45 + 10/*max int*/*3));
    sprintf(buffer,"TOTAL REQUESTS %d\nACCEPTED %d\nREJECTED %d\n",total, accepted, rejected);
    writeToLog(buffer);
    free(buffer);

    // Freeing bloom filters
    destroyHashTable(virusBloomFilters);
    free(monitorsArrey);

    // Destroying all the data structures used to save the travel statistics
    destroyCountryRegister(countriesAnswerRegister);
}