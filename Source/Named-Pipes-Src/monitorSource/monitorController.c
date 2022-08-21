/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "fileReader.h"
#include "CountryDirectory.h"
#include "Communicator.h"
#include "Utilities.h"
#include "LogFileWritter.h"
#include "SkipList.h"
#include "BloomFilter.h"
#include "HashTable.h"
#include "Virus.h"
#include "Person.h"
#include "StringDict.h"

static hashTable *virusesHT,*peopleHT;
static hashTable *countriesDict;
static int skipListHeight,initialized = 0;
static unsigned int bloomFilterSize;
static Communicator* c;
static hashTable *countriesDirs;

static unsigned int acceptedTravelRequests=0, rejectedTravelRequests=0;

#define INIT_MODE 0
#define ADD_MODE 1

#define MAX_STRING_SIZE 200 // A max buffer for string size

// Given a directory it calls the citizensFileReader of the 1st assigment, which reads the records and places the data in the program data strcuctures
void dummyCitizensFileReader(void* C_Dir, void *mode){

	/* The function has two modes
	   INIT_MODE: calls citizensFileReader() and reads the data of all the files
	   ADD_MODE: calls citizensFileReader() and reads the data ONLY for the files that have not been read (usefull for the addVaccinationRecords)
	*/
	if(*((char*)mode)!=INIT_MODE && *((char*)mode)!=ADD_MODE) return;

	// Getting the name of the directory from the countryDirectory structure, and checking accesability
	countryDirectory* countryDir = (countryDirectory*)C_Dir;
	char* countryDirName = (char*)getDirName(countryDir);
    DIR *directory = opendir(countryDirName);
    if(directory==NULL){
        printf("Directory %s ",(char*)countryDirName);
        if(errno == EACCES){
            printf("is unaccessible due to it's permisions!\n");
        }else if(errno == ENOENT){
            printf("does not exist!\n");
        }else{
            printf("error!\n");
        }
        return; // The file is unaccesable due to reasons, we can not continue
    }

	// Here all the files of the folder are read
    struct dirent *direntp;
    struct stat InodeInf;
    char *recPath=NULL,*countryDirW=NULL;
    while ( (direntp=readdir(directory)) != NULL ){// While there are files in the directory
        if(strcmp(direntp->d_name,".")==0 || strcmp(direntp->d_name,"..")==0) continue; //Skipping the entries '.' & '..'
		// The path of the file is created
        myStringCat(&countryDirW,countryDirName,"/"); // ex Greece + / = Greece/
        myStringCat(&recPath,countryDirW, direntp->d_name);// ex Greece/ + Greece-01 = Greece/Greece-01

		// Checking if the path leads to a regural file
        stat(recPath,&InodeInf);
        if((InodeInf.st_mode & S_IFMT) != S_IFREG) printf("Error: %s is not a regular file!\n",recPath);

		// Depending on the mode
		if (*((char*)mode) == INIT_MODE){
			// In the init mode we read the file no matter what
			insertFileToCountryDir(C_Dir,recPath);
		}else if(*((char*)mode) == ADD_MODE){
			// In the add mode we read the file only if it has not been read before
			if (checkFileInCountryDir(C_Dir,recPath) == 1){
				continue;
			}else{
				insertFileToCountryDir(C_Dir,recPath);
			}
		}

        if( citizensFileReader(recPath) != 0 ) printf("Initilazation error in file %s\n",recPath);
    }
    if(recPath != NULL) free(recPath);
    if(countryDirW != NULL) free(countryDirW);
    closedir(directory);
}

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

// Initilazes the monitors data strcutures and send any needed data to the travel monitor
int initilizeMonitor(char* communicatorFile, Communicator **mainCommunicatorPointer){
	c = openMonitorCommunicator(communicatorFile);
	*mainCommunicatorPointer = c;
    countriesDirs = newHashTable(1,cmpCountryDir,getDirName,voidStringHash,printCountryDir,freeDirString);//change 1
    int hashSize=1,bloomSize;
	unsigned int size=sizeof(int);

	// Recieving the size of the bloom filter
    recieveMessage(c,&bloomSize,&size);
	// The size of all bloom filters (in bytes)
	bloomFilterSize = bloomSize;

    // The default max size of all the skiplists (it was noted in piazza that it is good practice for a max limit to exist)
	skipListHeight = 10;
	
	// Three data structures are needed throughout the program
	
	// Two hashtables to save all the information about the virus and people so it can be easily accesible
	// The virus hash table has two skiplists and a bloom filter for each virus (more on Virus.h)
	virusesHT = newHashTable(hashSize,&virusCmp,&GetVirusName,&virusHash,&printVirus,&deleteVirus);
	// The people hash table simply has all the citizens for easy information
	peopleHT = newHashTable(hashSize,&PersonCmp,&GetPersonId,&PersonHash,&printPerson,&deleteVoidPerson);
	
	// Countries are a piece of infomation which will be repeted throughout the person records
	// Thus in order to minimize data duplication, it worths to save all the countries in a hashtable (since there will not be many)
	countriesDict = initializeStringDict(hashSize);

    char *buff=malloc(sizeof(char)*MAX_STRING_SIZE);
    size=-1;
	// Recieving the country directories
    do{// Do while since when we start we dont have values for the buff and size
        if(size!=-1){
            HashInsert(countriesDirs,newCountryFileName(buff));
        }
		size = MAX_STRING_SIZE;
        memset(buff,0,size);
        recieveMessage(c,buff,&size);
    }while(buff[0]!=EOF && size!=sizeof(char));
    free(buff);

	initialized = 1;// Data structures of the orogram are initialized


	// Traversing the countries directory HT using the citizen file reader function
	char mode = INIT_MODE;
    hashTraverse(countriesDirs, dummyCitizensFileReader, &mode);

	// Sending all the bloom filters to the parrent
    sendAllBloomFilters(c);

	return 0;
}

// SilentInsert in the record insertion function from the first assigment
int silentInsert(char* Id, char* lastName, char* firstName, char* country,unsigned int age, char* virusName,char vaccinated,char* date){
	if(initialized == 0) return -4; //If the data structures have not been initialized, error code is returned

	Person *person = (Person*)hashFind(peopleHT,Id);
	if(person==NULL){
		person = createPerson(Id,firstName,lastName,country,age,countriesDict);
		if(HashInsert(peopleHT,person)!=1){
			printf("error in pepole insertion!\n");
			return -2;
		}
	}else{
		if(FullPersonCmp(person,firstName,lastName,country,age)!=0){
			return -3;
		}
	}

	Virus *virus = (Virus*)hashFind(virusesHT,virusName);
	if(virus==NULL){
		virus = createVirus(virusName,skipListHeight,bloomFilterSize);
		if(HashInsert(virusesHT,virus)!=1){
			printf("error in virus insertion!\n");
			return -2;
		}
	}

	Person *tp;
	char *td;
	if(vaccinated==1){
		if( SkipListSearch(virus->not_vaccinated_persons,Id,&tp,&td) == 1 )
			return -1;//Already in not vaccinated skip list
		if( SkipListInsert(virus->vaccinated_persons,person,date) == -1) 
			return -1;//Already in vaccinated skip list
		if( BloomFilterInsert(virus->bloomFilter,(unsigned char*)Id)!=0 ) return -2;
	}else{
		if( SkipListSearch(virus->vaccinated_persons,Id,&tp,&td) == 1 )
			return -1;//Already in vaccinated skip list
		if( SkipListInsert(virus->not_vaccinated_persons,person,NULL) == -1)
			return -1;//Already in not vaccinated skip list
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
int addVaccinationRecords(char* country){
	countryDirectory *countryDir = (countryDirectory *)hashFind(countriesDirs, country);

	// If the country directory has been read in the past, we use the add mode of the dummyCitizensFileReader
	char mode = ADD_MODE;

	// If the country has not been read, it is the first time all files will be read, and we use the INIT_MODE.
	if ( countryDir==NULL ){
		countryDir = newCountryFileName(country);
		HashInsert(countriesDirs,countryDir);
		mode = INIT_MODE;
	}

	dummyCitizensFileReader(countryDir,&mode);

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

	//The memory from all data structures is freed	
	destroyHashTable(virusesHT);
	destroyHashTable(peopleHT);
	destroyStringDict(countriesDict);
	destroyHashTable(countriesDirs);

	// And the communicator is closed allong with its fifo files
	closeCommunicator(c);

	initialized = 0; //The program data structures are no longer initialized
	return 0;
}