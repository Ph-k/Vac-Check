/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
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

// Name of the monitor server executable
static const char* monitorServerExecutable = "./VacCheck_monitor";

// This struct holds all the necessary information of one monitor proecess
typedef struct Monitor{
    Communicator* communicator; // A communicator which uses pipes for bi-directonal communication
    hashTable* countries; // A string dictonery which with all the countries the monitor manages
    int countriesCount;
    pid_t pID; // It's process ID
} Monitor;

// Pointers to needed data structures (their scope is only in this file due to the static declaration)
static Monitor* monitorsArrey; // Arrey of all the information of the monitors
static hashTable* virusBloomFilters; // Hashtable which holds all the bloom filters
static countriesRegister* countriesAnswerRegister; // A counter structure used from the travelStats

// Program parameters (file scope)
static int numOfMonitors;

// A max buffer for string size (ex the maximum lenght of a virus name)
#define MAX_STRING_SIZE 100

// This port number and upwords to numofmonitors will be used for the sockets
#define INITIAL_PORT_NUMBER 1234

// This function forks the parents process and makes the children execute a monitor process
// Which basicaly creates/initiates a monitor process
pid_t spawnMonitor(int port, int numThreads, unsigned int cyclicBufferSize , unsigned int socketBufferSize, int bloomSize, hashTable* countiresDict, int countriesCount, char* countryInputDir){
    char **args;
    pid_t id = fork();
    if(id==0){// Child code...
        // ... Before the monitor is execed we create the arguments

        // Allocating an arrey big enough to fit the 12 standard arguments and the file path args
        args = malloc( sizeof(char*)*(12+countriesCount) );

        args[0] = strdup(monitorServerExecutable);

        //port arg
        args[1] = strdup("-p");
        args[2] = strIntDup(port);

        //num of threads arg
        args[3] = strdup("-t");
        args[4] = strIntDup(numThreads);

        // buffer size for socket arg
        args[5] = strdup("-b");
        args[6] = strUIntDup(socketBufferSize);

        // size of the cyclic buffer arg
        args[7] = strdup("-c");
        args[8] = strUIntDup(cyclicBufferSize);

        // The size of the bloom filter arg
        args[9] = strdup("-s");
        args[10] = strIntDup(bloomSize);


        // With this struct we pass the needed arguments to the hash traversing function
        struct dummyFunArgs{
            char **countryPaths;
            int index;
        };

        // See hashtraverse bellow
        void createCountriesArrey(void *countryString, void *VOIDdummyArgs){
            struct dummyFunArgs *dummyArgs = (struct dummyFunArgs *)VOIDdummyArgs;
            char *countryPath = NULL;
            // Concating input directory path with country directory path
            myStringCat(&countryPath,countryInputDir,(char*)countryString);
            dummyArgs->countryPaths[(dummyArgs->index)++] = countryPath;
        }

        struct dummyFunArgs dummyArgs;
        dummyArgs.countryPaths = args; // The arrey of arguments for the monitorServer
        dummyArgs.index = 11; // The index after which the file path arguments can be placed

        // The given string dictonery has the country file names wich have been alocated to this monitor
        // By traversing we place the file path arguments to the arguments arrey
        hashTraverse(countiresDict,createCountriesArrey,&dummyArgs);

        // End of arguments
        args[dummyArgs.index] = NULL;

        // Child exec part, the adress spaces of the program is overwritten with a monitor process
        execv(monitorServerExecutable,args);

        // If the program counter gets here, exec went wrong. Doing some memory cleanup
        for(int i=0; i<12; i++)
            if(args[i] != NULL ) free(args[i]);
        free(args);
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
int initializeProgramMonitors(char* input_dir, int numMonitors, int bloomSize, unsigned int bufferSize,unsigned int cyclicBufferSize ,int numThreads){
    int i,port=INITIAL_PORT_NUMBER;

    // Initializing data strcuture which is used for the travel request command
    countriesAnswerRegister = initilizeCountryRegister(10);

    // Creating an arrey to save the information of all the monitors
    monitorsArrey = malloc(sizeof(Monitor)*numMonitors);

    // Saving arguments to local/file variables
    numOfMonitors = numMonitors;

    // Allocating country directories to monitors happens here
    DIR *rootDir = opendir(input_dir);
    struct  dirent *direntp;
    char *recPath=NULL,*input_dirW=NULL;
    struct stat InodeInf;

    for(i=0; i<numMonitors; i++){
        monitorsArrey[i].countries = initializeStringDict(10); // A dictonary which saves the countries of the monitor
        monitorsArrey[i].countriesCount = 0;
    }

    i=0;
    myStringCat(&input_dirW,input_dir,"/"); // ex Data + / = Data/
    while ( (direntp=readdir(rootDir)) != NULL ){//While there are files/directories in the directory
        if(strcmp(direntp->d_name,".")==0 || strcmp(direntp->d_name,"..")==0) continue; //Skipping the entries '.' & '..'

        // The "full" path of the country directory is created
        myStringCat(&recPath,input_dirW, direntp->d_name); // ex Data/ + Greece = Data/Greece

        // If a non-folder file was found in the directory, there is nothing to do with it
        stat(recPath,&InodeInf);
        if((InodeInf.st_mode & S_IFMT) != S_IFDIR) printf("Error: %s is not a directory!\n",recPath);

        // The country folder is allocated to the i monitor
        insertString(monitorsArrey[i].countries,strdup(direntp->d_name)); // Saving that information
        (monitorsArrey[i].countriesCount)++;

        // This is how we circularly allocate the country directories to the monitors
        i = (i+1)%(numMonitors);
    }
    if(recPath != NULL) free(recPath);
    closedir(rootDir);

    // For all monitor procceses
    for(i=0; i<numMonitors; i++){
        //Creating monitors using fork and saving the monitors info
        monitorsArrey[i].pID = spawnMonitor(port,numThreads, cyclicBufferSize ,bufferSize, bloomSize,monitorsArrey[i].countries, monitorsArrey[i].countriesCount, input_dirW); // Forking & exec
        monitorsArrey[i].communicator = initClientCommunicator(&port,bufferSize); // Initialize communication above sockets
        port++;
    }
    if(input_dirW != NULL) free(input_dirW);

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
        pfds[i].fd = getCommunicationFd(monitorsArrey[i].communicator);
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
    //bloomFilterSum(vbf/*hashFind(virusBloomFilters,"INFLUENZA")*/,strdup("7819"));
    if( BloomFilterCheck(bf,(unsigned char*)citizenID) == 0 ){
        printf("REQUEST REJECTED – YOU ARE NOT VACCINATED (bloom=%d)\n",BloomFilterCheck(bf,(unsigned char*)citizenID));
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

    sendMessage(MonitorResponsibleForCountry->communicator,"addVaccinationRecords",sizeof(char)*21);

    // Creating and checking the path of the directory the new file is in
    char *recPath=NULL,*input_dirW=NULL;
    struct stat InodeInf;
    myStringCat(&input_dirW,input_dir,"/");
    myStringCat(&recPath,input_dirW, country);
    stat(recPath,&InodeInf);
    if((InodeInf.st_mode & S_IFMT) != S_IFDIR) printf("Error: %s is not a directory!\n",recPath);

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

        pfds[i].fd = getCommunicationFd(monitorsArrey[i].communicator);
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
    while(flag!=0){//While there are still sockets open that might send messages
		if(poll(pfds, numMonitors, -1) == -1) {printf("poll() parrent\n"); return -3;}//Poll is used to "wait" until one or more pipes have data
		for(i=0; i<numMonitors; i++){//For all the sockets
			if(pfds[i].revents & POLLIN){//If according to poll() the socket has data to be red
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
            // The exit behavior of the second assigment, killing all the children with sigkill without giving them the option to free memory
            if ( kill(monitorsArrey[i].pID,9) == -1) perror("Could not kill a child: ");
        }else{
            // The deafult/requested exit method, it tells the monitors to free their memory and terminate
            sendMessage(monitorsArrey[i].communicator,"exit\0",6*sizeof(char));
        }

        // Clossing communicator
        closeClientCommunicator(monitorsArrey[i].communicator);
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