#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include "./fileReader.h"
#include "CountryDirectory.h"
#include "../Communicator.h"
#include "../Utilities/Utilities.h"
#include "../Utilities/LogFileWritter.h"
#include "../DataStructures/SkipList.h"
#include "../DataStructures/BloomFilter.h"
#include "../DataStructures/HashTable.h"
#include "../Entities/Virus.h"
#include "../Entities/Person.h"
#include "../Entities/StringDict.h"

static hashTable *virusesHT,*peopleHT;
static hashTable *countriesDict;
static int skipListHeight,initialized = 0;
static unsigned int bloomFilterSize;
static Communicator* c;
static hashTable *countriesDirs;

static unsigned int acceptedTravelRequests=0, rejectedTravelRequests=0;

#define INIT_MODE 0
#define ADD_MODE 1

// A max buffer for string size
#define MAX_STRING_SIZE 200

// We asume that longest posible countryfile lenght is '/United_Kingdom_of_Great_Britain-101.txt\0'
#define MAX_COUNTRY_LENGHT 41


#define NUM_OF_MUTEXES 10
//Mutexes indexes on the mutex arrey used latter in the code
#define LIBRARY_FUNC_MUTEX 0
#define PEOPLE_HT_MUTEX 1
#define VIRUSES_HT_MUTEX 2
#define VIRUSES_VAC_SL_MUTEX 3
#define VIRUSES_BF_MUTEX 4
#define VIRUSES_NO_VAC_SL_MUTEX 5
#define DIRECTORIES_DICT_MUTEX 6
#define PERSON_MUTEX 7
#define CYCLIC_BUFFER_MUTEX 8
#define COUNTRY_DIRECTORIES_MUTEX 9

// Function used to send the bloom filters while traversing the virus HT
void sendOneBloomFilter(void* voidVirus,void* voidCommunicator){
	Virus *virus = (Virus*)voidVirus;
	Communicator *c = (Communicator*)voidCommunicator;
	const char *bitArrey;
	unsigned int bitArreySize;
	sendMessage(c,virus->name,strlen(virus->name)*sizeof(char)); // Sending the virus of the bloom filter
	getBloomFilterBitArrey(virus->bloomFilter,&bitArrey,&bitArreySize); // Getting the bloom filter
	sendMessage(c,bitArrey,bitArreySize); // Sending the bloom filter
}

void sendAllBloomFilters(Communicator *c){
	// In order to send all the bloom filters we simply traverse the viruses HT with the above function
	hashTraverse(virusesHT, sendOneBloomFilter,c);
	// An EOF signals the end of the sending of bloom filters
	char endC = EOF;
	sendMessage(c,&endC,sizeof(char));
}

// This structure contains the arguments for the thread function, it is passed casted as void*
struct dummyTreadFunArgs{
	// Each thread has
	pthread_mutex_t *mutexes; // The arrey of mutexes, which in combination with the above defines provides access to all the mutexes
	char* cyclicBuffer; // A pointer to the cyclicBuffer
	int cyclicBufferSize; // The size of the cyclic buffer in bytes (not in file paths)
	sem_t *cyclicBufferEmpty; // A semaphore to wait while there are no empty slots on the buffer
	sem_t *cyclicBufferFull; // A semaphore to wait while there are no records one the buffer
	int cyclicBufferIndex; // The current index to the last filepath record
	int cyclicBufferRecordSize; // The size of a filepath in bytes
};

// This function impliments the execution of the threads, which read and initliaze all the data structures using mutexes
void* threadInitialazation(void* structArg){
	struct dummyTreadFunArgs* args = (struct dummyTreadFunArgs*)structArg;

	char loopCondition=1,*filePath=NULL;

	// While there is data to be read on the cyclic buffer
	while(loopCondition == 1){
		sem_wait(args->cyclicBufferFull); // Waiting until there is data to be read on the cyclic buffer
		pthread_mutex_lock(&(args->mutexes[CYCLIC_BUFFER_MUTEX])); // The removal of a record from the buffer is C.S.

		// If the index of the last record in the buffer is 0 and the buffer is empty, there are no more records
		// Notes: 
		// 1) While there is data on the buffer the index is never 0, if only one record is on the buffer the index is set to 1.
		// 2) If all the threads have emptied the buffer and the main thread has not places new records,
		//    the threads will be stuck on the cyclicBufferFull first.
		if(args->cyclicBufferIndex == 0 && checkIfNull(args->cyclicBuffer,args->cyclicBufferSize) == 1 ){ // Buffer empty case
			// Waking all other threads by dealocating locked sem and mutex. So they can also check the buffer and terminate.
			pthread_mutex_unlock(&(args->mutexes[CYCLIC_BUFFER_MUTEX]));
			sem_post(args->cyclicBufferFull);
			loopCondition = 0;
			break;
		}
		// One record will be read
		(args->cyclicBufferIndex)--;

		// Getting the record/filepath (will terminate with a '\0')
		filePath = strdup(args->cyclicBuffer + args->cyclicBufferRecordSize*args->cyclicBufferIndex);

		// Removing the read record from the buffer
		memset(args->cyclicBuffer + args->cyclicBufferRecordSize*args->cyclicBufferIndex,0,args->cyclicBufferRecordSize);

		// Dealocating locked sem and mutex for access to the cyclic buffer
		pthread_mutex_unlock(&(args->mutexes[CYCLIC_BUFFER_MUTEX]));
		sem_post(args->cyclicBufferEmpty);

		// Reading the data from the file, any access to common data structures is protected with mutexes
		if( citizensFileReader(filePath,args->mutexes) != 0 ) printf("Initilazation error in file %s\n",filePath);

		// Freeing memory of the strduped filepath string
		free(filePath);
	}

	return NULL;
}

// This function is executed from the main thread, it creates the threads executing the above function, and then fills the buffer
int saveFileDataToDataStructs(int numThreads,int cyclicBufferCount, char** countryArgs, char mode){
	int i;
	struct dummyTreadFunArgs dummyThreadArgs;

	// Finding the lengthier filepath, in order to allocated enough space in the cyclic buffer
	dummyThreadArgs.cyclicBufferRecordSize = 0;
   	for(i=0; countryArgs[i] != NULL; i++){
		if( dummyThreadArgs.cyclicBufferRecordSize < strlen(countryArgs[i]) )
			dummyThreadArgs.cyclicBufferRecordSize = strlen(countryArgs[i]);
    }
	// The maximum path size of the cyclic buffer is the biggest directory path + the longest countryfile name posible
	dummyThreadArgs.cyclicBufferRecordSize += MAX_COUNTRY_LENGHT;

	// Initilazing the arguments of the threads
	dummyThreadArgs.cyclicBufferIndex = 0;
	dummyThreadArgs.cyclicBufferSize = dummyThreadArgs.cyclicBufferRecordSize*cyclicBufferCount*sizeof(char);
	dummyThreadArgs.cyclicBuffer = malloc(dummyThreadArgs.cyclicBufferSize);
	memset(dummyThreadArgs.cyclicBuffer,0,dummyThreadArgs.cyclicBufferSize);

	// Creating all the needed mutexes and placing them in an arrey
	int mutexesCount = NUM_OF_MUTEXES;
	dummyThreadArgs.mutexes = malloc(sizeof(pthread_mutex_t )*mutexesCount);
	for(i=0; i<mutexesCount; i++){
		pthread_mutex_init(dummyThreadArgs.mutexes + i, NULL);
	}

	sem_t *cyclicBufferFull = malloc(sizeof(sem_t)), // Semaphore to wait while there are no records one the buffer
	      *cyclicBufferEmpty = malloc(sizeof(sem_t)); // Semaphore to wait while there are no empty slots on the buffer
	dummyThreadArgs.cyclicBufferFull = cyclicBufferFull;
	dummyThreadArgs.cyclicBufferEmpty = cyclicBufferEmpty;
	pthread_t threads[numThreads];

	// Creating the two needed semaphores for the buffer
	if ( 
		sem_init(cyclicBufferFull,0,0) != 0
		||
		sem_init(cyclicBufferEmpty,0,cyclicBufferCount) != 0
	) {perror("Monitor: sem_init() "); return -1;}

	initialized = 1;// Data structures of the program are to be initialized now from the threads

	// Creating all the needed threads and starting their execution
	for(i=0; i<numThreads; i++){
		if(pthread_create(threads + i, NULL, &threadInitialazation, &dummyThreadArgs) != 0)
			perror("Monitor: Thread creation failed ");
	}

	// The main thread will now fill the cyclic buffer
	struct dirent *direntp;
	DIR *cDIR;
	struct stat InodeInf;
	char *recPath=NULL,*countryDirW=NULL;
	countryDirectory* countryDir;
	// For all the given country directories
	for(i=0; countryArgs[i] != NULL; i++){
		// Note: the following hashtable is accesed only from the main thread, so no mutex needed
		// Inserting the country directory to the structure responsable for "remembering" the read files if needed.
		countryDir = hashFind(countriesDirs,countryArgs[i]);
		if(countryDir == NULL){
			countryDir = newCountryFileName(countryArgs[i]);
			HashInsert(countriesDirs,countryDir);
		}

		cDIR = opendir(countryArgs[i]);
		if(cDIR == NULL) perror("Monitor: Dir() ");
		direntp=readdir(cDIR);
		while ( direntp != NULL ){// While there are files in the directory
			// We skip the entries '.' & '..'
			if(strcmp(direntp->d_name,".")==0 || strcmp(direntp->d_name,"..")==0) { direntp=readdir(cDIR); continue; }

			// The path of the file is created
			myStringCat(&countryDirW,countryArgs[i],"/"); // ex Greece + / = Greece/
			myStringCat(&recPath,countryDirW, direntp->d_name);// ex Greece/ + Greece-01 = Greece/Greece-01

			// Checking if the path leads to a regural file
			stat(recPath,&InodeInf);
			if((InodeInf.st_mode & S_IFMT) != S_IFREG) printf("Error: %s is not a regular file!\n",recPath);

			// Depending on the mode
			if ( mode == INIT_MODE){
				// In the init mode we read the files and their data no matter what
				insertFileToCountryDir(countryDir,recPath);
			}else if( mode == ADD_MODE){
				// In the add mode we read the file only if it has not been read before
				if (checkFileInCountryDir(countryDir,recPath) == 1){
					// File read, insertion will not happen
					direntp=readdir(cDIR);
					continue;
				}else{
					insertFileToCountryDir(countryDir,recPath);
				}
			}
			// Note: the countryDir structure is a node of the countriesDirs HT. Both are not accessed concurrently by any other thread

			// Now that the main thread has "produced" a file path entry for the cyclic buffer,
			// the only thing left, is for it to be placed on the cyclic buffer
			// Waiting until there are empty slots on the buffer
			sem_wait(cyclicBufferEmpty);
			// As in the threadInitialazation, access to the the cyclic buffer is considered C.S. protected with a mutex
			pthread_mutex_lock(&(dummyThreadArgs.mutexes[CYCLIC_BUFFER_MUTEX]));
			// Coppying the file path string to the right location on the buffer using memcpy
			memcpy(
				dummyThreadArgs.cyclicBuffer + dummyThreadArgs.cyclicBufferRecordSize*dummyThreadArgs.cyclicBufferIndex,
				recPath,
				strlen(recPath)*sizeof(char)
			);
			(dummyThreadArgs.cyclicBufferIndex)++;// A record has been added to the buffer
			// Unlocking mutex, and posting on semaphore to give access of the C.S. to another thread
			pthread_mutex_unlock(&(dummyThreadArgs.mutexes[CYCLIC_BUFFER_MUTEX]));
			sem_post(cyclicBufferFull);

			direntp=readdir(cDIR);
    	}
		closedir(cDIR);
	}
	// After all the records have been placed to the cyclic buffer,
	// this post to the semaphore will triger at some point one thread to realize that no more records will be writen,
	// which will then post the semaphore again to signal to another thread that no more records will be writen etc.
	sem_post(dummyThreadArgs.cyclicBufferFull);

	if(recPath!=NULL) free(recPath);
	if(countryDirW!=NULL) free(countryDirW);

	// Waiting for all threads to empty the buffer, and terminate
	for(i=0; i<numThreads; i++){
		if(pthread_join(threads[i], NULL) != 0)
			perror("Monitor: Thread join failed ");
	}

	// Deleting all the mutexes, the semaphores, and the dynamicaly allocated memory
	for(i=0; i<mutexesCount; i++){
		pthread_mutex_destroy(dummyThreadArgs.mutexes + i);
	}
	sem_destroy(cyclicBufferFull);
	sem_destroy(cyclicBufferEmpty);
	free(cyclicBufferFull);
	free(cyclicBufferEmpty);
	free(dummyThreadArgs.cyclicBuffer);
	free(dummyThreadArgs.mutexes);
	return 0;
}

// Initilazes the monitors data strcutures and sends any needed data to the travel monitor
int initilizeMonitor(int port,int numThreads,int cyclicBufferCount, unsigned int bloomSize, char** countryArgs, unsigned int socketBufferSize, Communicator **mainCommunicatorPointer){
	bloomFilterSize = bloomSize;

	c = initServerCommunicator(port,socketBufferSize); // Initialazing socket communication
	*mainCommunicatorPointer = c;
    countriesDirs = newHashTable(10,cmpCountryDir,getDirName,voidStringHash,printCountryDir,freeDirString);
    int hashSize=1;

    // The default max size of all the skiplists (it was noted in piazza that it is good practice for a max limit to exist)
	skipListHeight = 10;
	
	// Three data structures are needed throughout the program
	
	// Two hashtables to save all the information about the virus and people so it can be easily accesible
	// The virus hash table has two skiplists and a bloom filter for each virus (more on Virus.h)
	virusesHT = newHashTable(hashSize,&virusCmp,&GetVirusName,&virusHash,&printVirus,&deleteVirus);
	// The people hash table simply has all the citizens for easy information
	peopleHT = newHashTable(hashSize,&PersonCmp,&GetPersonId,&PersonHash,&printPerson,&deleteVoidPerson);
	
	// Countries are a piece of infomation which will be repeated throughout the person records
	// Thus in order to minimize data duplication, it worths to save all the countries in a hashtable (since there will not be many)
	countriesDict = initializeStringDict(hashSize);

	// This function now will create threads which will then fill the datastructures with the data from ALL (INIT_MODE) the files
	saveFileDataToDataStructs(numThreads, cyclicBufferCount, countryArgs, INIT_MODE);

	// Sending all the bloom filters to the parrent
	sendAllBloomFilters(c);

	return 0;
}

// SilentInsert is the record insertion function from the first assigment
int silentInsert(char* Id, char* lastName, char* firstName, char* country,unsigned int age, char* virusName,char vaccinated,char* date, pthread_mutex_t *mutexes){
	if(initialized == 0) return -4; //If the data structures have not been initialized, error code is returned

	// Note: All the common/concurrently accessed data structures are protected with their own mutexes,
	// ensuring respectable efficiency and safety.

	// Finding (or creating) the person maintioned in the record. (Error if person data is invalid)
	pthread_mutex_lock(&(mutexes[PEOPLE_HT_MUTEX]));
	Person *person = (Person*)hashFind(peopleHT,Id);
	if(person==NULL){
		pthread_mutex_lock(&(mutexes[PERSON_MUTEX]));
		person = createPerson(Id,firstName,lastName,country,age,countriesDict);
		pthread_mutex_unlock(&(mutexes[PERSON_MUTEX]));

		if(HashInsert(peopleHT,person)!=1){
			printf("error in pepole insertion!\n");
			pthread_mutex_unlock(&(mutexes[PEOPLE_HT_MUTEX])); // The functions ends, unlocking mutex to avoid deadlock
			return -2;
		}
	}else{
		if(FullPersonCmp(person,firstName,lastName,country,age)!=0){
			pthread_mutex_unlock(&(mutexes[PEOPLE_HT_MUTEX])); // The functions ends, unlocking mutex to avoid deadlock
			return -3;
		}
	}
	pthread_mutex_unlock(&(mutexes[PEOPLE_HT_MUTEX]));

	// Same thing for virus, searching for the virus in the record (creating if it does not exist). All protected but mutexes
	pthread_mutex_lock(&(mutexes[VIRUSES_HT_MUTEX]));
	Virus *virus = (Virus*)hashFind(virusesHT,virusName);
	if(virus==NULL){

		// The virus creation needs to access a lot of data structures
		// The following loop ensures that all mutexes will be aquiered before creating the virus,
		// While avoiding deadlocks due to a thread holding one mutex while beeing locked on mutex held from another process
		char acquired = 0;
		while(acquired == 0){// While not all mutexes have aquired
			// Trying to lock the vaccinated S.L. mutex
			if( pthread_mutex_trylock(&(mutexes[VIRUSES_VAC_SL_MUTEX])) !=0 ){
				continue; // If the above fails we try again on the next iteration
			}

			// If the vaccinated S.L. mutex was aquired, the thread will now try to aquire the next mutex
			if( pthread_mutex_trylock(&(mutexes[VIRUSES_NO_VAC_SL_MUTEX])) !=0 ){
				// If the above fails, any aquired mutuxes are unlocked to avoid deadlocks. And the thread retries to aquire
				pthread_mutex_unlock(&(mutexes[VIRUSES_VAC_SL_MUTEX]));
				continue;
			}

			// Same as above for the bloom filter mutex
			if( pthread_mutex_trylock(&(mutexes[VIRUSES_BF_MUTEX])) != 0 ){
				pthread_mutex_unlock(&(mutexes[VIRUSES_VAC_SL_MUTEX]));
				pthread_mutex_unlock(&(mutexes[VIRUSES_NO_VAC_SL_MUTEX]));
				continue;
			}

			acquired = 1; // If we reached here all the mutexes where aquired, the loop will terminate
		}

		// Creating the virus, and exiting the C.S.
		virus = createVirus(virusName,skipListHeight,bloomFilterSize);
		pthread_mutex_unlock(&(mutexes[VIRUSES_VAC_SL_MUTEX]));
		pthread_mutex_unlock(&(mutexes[VIRUSES_NO_VAC_SL_MUTEX]));
		pthread_mutex_unlock(&(mutexes[VIRUSES_BF_MUTEX]));

		if(HashInsert(virusesHT,virus)!=1){
			printf("error in virus insertion!\n");
			pthread_mutex_unlock(&(mutexes[VIRUSES_HT_MUTEX]));
			return -2;
		}
	}
	pthread_mutex_unlock(&(mutexes[VIRUSES_HT_MUTEX]));

	Person *tp;
	char *td;
	// Inserting vaccination information for the given record
	if(vaccinated==1){
		pthread_mutex_lock(&(mutexes[VIRUSES_VAC_SL_MUTEX]));
		if( SkipListSearch(virus->not_vaccinated_persons,Id,&tp,&td) == 1 ){
			pthread_mutex_unlock(&(mutexes[VIRUSES_VAC_SL_MUTEX]));
			return -1;//Already in not vaccinated skip list
		}
		if( SkipListInsert(virus->vaccinated_persons,person,date) == -1){
			pthread_mutex_unlock(&(mutexes[VIRUSES_VAC_SL_MUTEX]));
			return -1;//Already in vaccinated skip list
		}
		pthread_mutex_unlock(&(mutexes[VIRUSES_VAC_SL_MUTEX]));

		pthread_mutex_lock(&(mutexes[VIRUSES_BF_MUTEX]));
		if( BloomFilterInsert(virus->bloomFilter,(unsigned char*)Id)!=0 ){
			pthread_mutex_unlock(&(mutexes[VIRUSES_BF_MUTEX]));
			return -2;
		}
		pthread_mutex_unlock(&(mutexes[VIRUSES_BF_MUTEX]));
	}else{
		pthread_mutex_lock(&(mutexes[VIRUSES_NO_VAC_SL_MUTEX]));
		if( SkipListSearch(virus->vaccinated_persons,Id,&tp,&td) == 1 ){
			pthread_mutex_unlock(&(mutexes[VIRUSES_NO_VAC_SL_MUTEX]));
			return -1;//Already in vaccinated skip list
		}
		if( SkipListInsert(virus->not_vaccinated_persons,person,NULL) == -1){
			pthread_mutex_unlock(&(mutexes[VIRUSES_NO_VAC_SL_MUTEX]));
			return -1;//Already in not vaccinated skip list
		}
		pthread_mutex_unlock(&(mutexes[VIRUSES_NO_VAC_SL_MUTEX]));
	}
	
	return 0;
}

// Impliments part of the monitor for the travelRequest command
int travelRequest(char* citizenID,char* virusName, char* requestedDate){
	Virus *virus = (Virus*)hashFind(virusesHT,virusName);
	if(virus==NULL){
		printf("MONITOR: NO SUCH VIRUS!\n");
		return -1;
	}

	Person *person;
	char* date;

	char* answer;
	if( SkipListSearch(virus->vaccinated_persons,citizenID,&person,&date)==1 ){
		// If the person is vaccinated he will exist in the vaccinated skip-list of the virus

		// Sending the awnser to the travelMonitor process depending on the vaccination date
		if(dateDiffernceInMonths(requestedDate,date)<=6 && dateCmp(requestedDate,date) >= 0 ){
			answer = strdup("YES\0");
			sendMessage(c,answer,strlen(answer)*sizeof(char));
			free(answer);
			acceptedTravelRequests++;//Counting the awnser
			return 1;
		}else{
			answer = strdup("YES_BUT_NO\0");
			sendMessage(c,answer,strlen(answer)*sizeof(char));
			free(answer);
			rejectedTravelRequests++;//Counting the awnser
			return 3;
		}
	}else if( SkipListSearch(virus->not_vaccinated_persons,citizenID,&person,&date)==1 ){
		// If the person is not vaccinated he will exist in the vaccinated skip-list of the virus

		// Sending the awnser to the travelMonitor process
		answer = strdup("NO\0");
		sendMessage(c,answer,strlen(answer)*sizeof(char));
		free(answer);
		rejectedTravelRequests++;//Counting the awnser
		return 0;
	}else{
		// In any other case we have no info for this citizen about this virus
		// And the requset is considered rejected
		answer = strdup("NO\0");
		sendMessage(c,answer,strlen(answer)*sizeof(char));
		free(answer);
		rejectedTravelRequests++;
		return 2;
	}
	free(answer);
	return 0;
}

// Impliments part of the monitor for the addVaccinationRecords command
int addVaccinationRecords(char* country, int numThreads, int cyclicBufferCount){
	// The same function used in the initilazation is re-used, so the country arguments needs to be on an arg arrey format
	char **countryArgs = malloc(sizeof(char*)*2);
	countryArgs[0] = country;
	countryArgs[1] = NULL;
	// The function will read all the data in ADD_MODE
	saveFileDataToDataStructs(numThreads, cyclicBufferCount, countryArgs, ADD_MODE);
	free(countryArgs);

	// After the data is read, we send the updated bloom filters
	sendAllBloomFilters(c);
	return 0;
}

// Impliments part of the monitor for the vaccineStatusAll command
int vaccineStatusAll(char* citizenID){

	// Nested fuction which finds and sends the result
	void PrintVaccineStatus(void* v, void* citizenID){
		Virus *virus = (Virus*)v;
		Person *person;
		char* date;
		if( SkipListSearch(virus->vaccinated_persons,citizenID,&person,&date)==1 ){
			// If the citizen is found in the vaccinated skip list, we send the information to the parent
			sendMessage(c,"ForVirus",8*sizeof(char));// Informoning the parrent that we are going to send vaccine data
			sendMessage(c,virus->name,strlen(virus->name)*sizeof(char));// Sending the virus name
			sendMessage(c,date,strlen(date)*sizeof(char));// And the date
		}else if( SkipListSearch(virus->not_vaccinated_persons,citizenID,&person,&date)==1 ){
			// If the citizen is found in the non-accinated skip list, we send the information to the parent
			sendMessage(c,"ForVirus",8*sizeof(char));// Informoning the parrent that we are going to send vaccine data
			sendMessage(c,virus->name,strlen(virus->name)*sizeof(char));// Sending the virus name
			sendMessage(c,"NO",2*sizeof(char));// Sending the indformation that the citizen is not vaccinated
		}/*else{
			There is no info for this citizen id, nothing to send
		}*/
	}

	// All the viruses are traversed and have their skip-list information searched about the given person
	hashTraverse(virusesHT,&PrintVaccineStatus,citizenID);
	return 0;
}

// Impliments part of the monitor for the searchVaccinationStatus command
int searchVaccinationStatus(char* citizenID){
	Person *person = (Person*)hashFind(peopleHT,citizenID);
	if(person==NULL){//If this particular person does not exist, there is nothing to send
		sendMessage(c,"NoCitizenWithThisId",19*sizeof(char));
		return 0;
	}

	// If this citizen exists, we send the informatoin of the citizen
	sendMessage(c,"GotCitizenWithThisId",20*sizeof(char));
	sendPerson(person,c);

	// Along with his/her vaccination status information
	vaccineStatusAll(citizenID);

	// Informing the parrent that we sent all the data we had
	sendMessage(c,"GaveEvrythingThisId",19*sizeof(char));
	return 0;
}

// Prints all the requested data to the log file
void createLogFile(){
	// Using the dummyCountyLogWritter we can write the information of the countriesDict HT to the log file
	hashTraverse(countriesDict,dummyCountyLogWritter,NULL);

	// Finnaly the requests counters are printed to the log file
	char* buffer = malloc(sizeof(char)*(45 + 10/*max int*/*3));
    sprintf(buffer,"TOTAL REQUESTS %d\nACCEPTED %d\nREJECTED %d\n",
	acceptedTravelRequests+rejectedTravelRequests, acceptedTravelRequests, rejectedTravelRequests);
    writeToLog(buffer);
	free(buffer);
}

// This function terminates the monitor process by freeing all its memory
// (note: of course this function will never be executed when sigkill is recieved, 
// if you have not understand the existace of this function see the "restFullyexit" command in the travelMonitor
int terminateMonitor(){
	if(initialized == 0) return -1; //If the data structures have not been initialized, error code is returned

	// Creating a log file before exiting as requested
	createLogFile();

	//The memory from all data structures is freed	
	destroyHashTable(virusesHT);
	destroyHashTable(peopleHT);
	destroyStringDict(countriesDict);
	destroyHashTable(countriesDirs);

	// And the communicator is closed allong with its fifo files
	closeServerCommunicator(c);

	initialized = 0; //The program data structures are no longer initialized
	return 0;
}