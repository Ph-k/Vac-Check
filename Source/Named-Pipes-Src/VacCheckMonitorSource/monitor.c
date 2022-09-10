/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "Communicator.h"
#include "Utilities.h"
#include "Virus.h"

#include "fileReader.h"
#include "monitorController.h"
#include "monitorSignalHandlers.h"

int main(int argc, char **argv){
	setSignalHandlers();
    if(argc!=2) {printf("monitor: check arguments format!\n"); return -1;}

	Communicator *c;
    initilizeMonitor(argv[1],&c);// Initializing the program data structures and the communacator pipes

    unsigned int maxInputStingSize = 250;
    char execution = 1,*command,
	*citizenID=malloc(sizeof(char)*maxInputStingSize),
	*country=malloc(sizeof(char)*maxInputStingSize),
	*virusName=malloc(sizeof(char)*maxInputStingSize),
	*date = malloc(sizeof(char)*strlen("31-12-9999 ")),
	*inputBuffer = malloc(sizeof(char) * maxInputStingSize);

    while( execution == 1 ){// In this loop the monitor executes commands the parent/travel monitor process requests
		maxInputStingSize=250;

		// Checking signal flags, which change from the signal handlers
		if(SigUsr1_Flag>0){
			printf("I am a monitor and I have recievied a Usr1 signal\n");
			SigUsr1_Flag--;
			sendMessage(c,"ACK",sizeof(char)*3);// Sending acknowledgment of the signal to the travel monitor
			recieveMessage(c,country,&maxInputStingSize);// Reading the country directory of the new files
			addVaccinationRecords(country); // Calling the function which impliments the monitor part of the command
		}
		if(SigIntQuit_Flag>0){
			printf("I am a monitor an  have recievied a int or quit signal\n");
			SigIntQuit_Flag--;
			// Writting the needed information to the log file
			createLogFile();
		}

		// Waiting for a command from the parrent process
        if( recieveMessage(c,inputBuffer,&maxInputStingSize) == -2){
			continue; //Read interupted from a signal, the first thing that will happen, is the checking of the signal flags
		}

		command = strtok(inputBuffer, " ");// Getting first argument of parent input, the command

		//The structure of if else statements executes the requested command form the parrent/travel monitor
		if( strcmp(command,"travelRequest") == 0 ){

			// Receiving citizenID parameter from travel monitor
			maxInputStingSize=250;
			memset(citizenID,'\0',maxInputStingSize);
			recieveMessage(c,citizenID,&maxInputStingSize);

			// Receiving virusName parameter from travel monitor
			maxInputStingSize=250;
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
				printf("MONITOR: CORRUPTED PIPE DATA!\n");
			}

		}else if( strcmp(command,"searchVaccinationStatus") == 0 ){
			// Receiving citizenID parameter from travel monitor
			maxInputStingSize=250;
			memset(citizenID,'\0',maxInputStingSize);
			recieveMessage(c,citizenID,&maxInputStingSize);

			if(citizenID != NULL){
				// Calling the function of the controler which impliments the commands functionality
				searchVaccinationStatus(citizenID);
			}else{
					printf("MONITOR: CORRUPTED PIPE DATA!\n");
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