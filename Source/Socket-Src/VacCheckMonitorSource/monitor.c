/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "fileReader.h"
#include "Communicator.h"
#include "Virus.h"
#include "monitorController.h"
#include "Utilities.h"

// The maximum length of a command given from the travel client through the socket
#define MAX_INPUT_BUFFER 250

int main(int argc, char **argv){

	if( argc < 9 ) {printf("Monitor: not enough arguments! (you gave just %d)\n",argc); return -1;}

	// Reading the arguments in the order described in the assignment
	// Since we do not know how many path arguments will be given
	if( strcmp(argv[1],"-p") !=0 ) {printf("Monitor: no -p agrument found!\n"); return -1;}
	int port = atoi(argv[2]);

	if( strcmp(argv[3],"-t") !=0 ) {printf("Monitor: no -t agrument found!\n"); return -1;}
	int numThreads = atoi(argv[4]);

	if( strcmp(argv[5],"-b") !=0 ) {printf("Monitor: no -b agrument found!\n"); return -1;}
	int socketBufferSize = atoi(argv[6]);

	if( strcmp(argv[7],"-c") !=0 ) {printf("Monitor: no -c agrument found!\n"); return -1;}
	int cyclicBufferSize = atoi(argv[8]);

	if( strcmp(argv[9],"-s") !=0 ) {printf("Monitor: no -s agrument found!\n"); return -1;}
	int sizeOfBloom = atoi(argv[10]);

	// All the arguments after the 10th are the paths to the country directory files

	Communicator *c;
	// Initializing the program data structures and the communacator over sockets
    initilizeMonitor(port,numThreads,cyclicBufferSize,sizeOfBloom,&(argv[11]),socketBufferSize,&c);

    unsigned int maxInputStingSize = MAX_INPUT_BUFFER;
    char execution = 1,*command,
	*citizenID=malloc(sizeof(char)*maxInputStingSize),
	*country=malloc(sizeof(char)*maxInputStingSize),
	*virusName=malloc(sizeof(char)*maxInputStingSize),
	*date = malloc(sizeof(char)*strlen("31-12-9999 ")),
	*inputBuffer = malloc(sizeof(char) * maxInputStingSize);

    while( execution == 1 ){// In this loop the monitor executes commands the parent/travel monitor process requests
		maxInputStingSize=MAX_INPUT_BUFFER;

		// Waiting for a command from the parrent process
        if( recieveMessage(c,inputBuffer,&maxInputStingSize) == -2){
			continue; //Read interupted from a signal (project 2)
			// NOTE: THE SIGNAL HANDLING CASE OF recieveMessage WILL NEVER EXECUTE SINCE THERE ARE NO SIGNAL HANDLERS USED IN THIS PROJECT
		}

		command = strtok(inputBuffer, " ");// Getting first argument of parent input, the command

		//The structure of if else statements executes the requested command form the parrent/travel monitor
		if( strcmp(command,"travelRequest") == 0 ){

			// Receiving citizenID parameter from travel monitor
			maxInputStingSize=MAX_INPUT_BUFFER;
			memset(citizenID,'\0',maxInputStingSize);
			recieveMessage(c,citizenID,&maxInputStingSize);

			// Receiving virusName parameter from travel monitor
			maxInputStingSize=MAX_INPUT_BUFFER;
			memset(virusName,'\0',maxInputStingSize);
			recieveMessage(c,virusName,&maxInputStingSize);

			// Receiving date parameter from travel monitor
			maxInputStingSize=sizeof(char)*strlen("31-12-9999 ");
			memset(date,'\0',maxInputStingSize);
			recieveMessage(c,date,&maxInputStingSize);

			// All arguments are needed for this command
			if(citizenID != NULL && virusName !=NULL && date != NULL){
				// Calling the function of the controler which impliments the commands functionality
				travelRequest(citizenID,virusName,date);
			}else{
				printf("MONITOR: CORRUPTED SOCKET DATA!\n");
			}

		}else if( strcmp(command,"addVaccinationRecords") == 0){
			maxInputStingSize=MAX_INPUT_BUFFER;
			memset(citizenID,'\0',maxInputStingSize);
			recieveMessage(c,country,&maxInputStingSize);// Reading the country directory of the new files

			if(citizenID != NULL){
				// Calling the function of the controler which impliments the commands functionality
				addVaccinationRecords(country,numThreads,cyclicBufferSize); // Calling the function which impliments the monitor part of the command
			}else{
					printf("MONITOR: CORRUPTED SOCKET DATA!\n");
			}
		}else if( strcmp(command,"searchVaccinationStatus") == 0 ){
			// Receiving citizenID parameter from travel monitor
			maxInputStingSize=MAX_INPUT_BUFFER;
			memset(citizenID,'\0',maxInputStingSize);
			recieveMessage(c,citizenID,&maxInputStingSize);

			if(citizenID != NULL){
				// Calling the function of the controler which impliments the commands functionality
				searchVaccinationStatus(citizenID);
			}else{
					printf("MONITOR: CORRUPTED SOCKET DATA!\n");
			}

		}else if( strncmp(command,"exit",4) == 0 ){
			//The program is terminated by breaking out of the execution loop
			execution =  0;
		}else{
			printf("Monitor recieved an unvalid command! (%s)\n",command);
		}
	}
	free(citizenID);
	free(country);
	free(virusName);
	free(date);
    free(inputBuffer);

	// If the monitor was not killed and was told to terminate, the memory of the monitor is freed and then exits
    terminateMonitor();
    return 0;
}