#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "Communicator.h"
#include "Utilities.h"
#include "HashTable.h"
#include "travelController.h"
#include "travelSignalHandlers.h"

int main(int argc, char **argv){
    if(argc!=9) {printf("Travel monitor: check arguments format!\n"); return -1;}

    setSignalHandlers();

    int i,numMonitors = -1, bloomSize = -1;
    unsigned int bufferSize = 0;
    char *input_dir=NULL;

    for(i=1; i<argc; i++){//Reading all the given arguments
        if( strcmp(argv[i],"-m") == 0 ){
            numMonitors = atoi(argv[++i]);
        }else if( strcmp(argv[i],"-b") == 0){
            bufferSize = atoi(argv[++i]);
        }else if( strcmp(argv[i],"-s") == 0){
            bloomSize = atoi(argv[++i]);
        }else if( strcmp(argv[i],"-i") == 0){
            input_dir = argv[++i];
        }else{
            printf("Argument '%s' is invalid\n",argv[i]);
        }
    }

    if(numMonitors<=0 || bufferSize<=0 || bloomSize<=0 || input_dir==NULL){
		printf("One or many of the program parameters was mistyped or not given at all!\n");
		return -2;
	}
    printf("Parameters: numMonitors=%d bufferSize=%u sizeOfBloom=%d input_dir=%s\n", numMonitors, bufferSize, bloomSize, input_dir);

    // Checking if the input_dir exists and is accessible
    DIR *rootDir = opendir(input_dir);
    if(rootDir==NULL){
        printf("Directory %s ",input_dir);
        // Printing what it the problem with the directory
        if(errno == EACCES){
            printf("is unaccessible due to it's permisions!\n");
        }else if(errno == ENOENT){
            printf("does not exist!\n");
        }else{
            printf("error!\n");
        }
        closedir(rootDir);
        return -3; // Terminating program since we won't be able to acces any data
    }
    //Otherwise the directory is closed and the execution continiues normaly
    closedir(rootDir);

    // This function initializes all needed data structures, and creates the monitor processes along with the needed communication pipes
    initializeProgramMonitors(input_dir, numMonitors, bloomSize, bufferSize);

    // After the initialazation of the travelMonnitor/parrent, the monitos/children have to send the bloom filters
    recieveBloomFilters(numMonitors, bloomSize);

    size_t maxInputStingSize = 250;// The maximum input length
    char execution = 1,*command,*citizenID,*country,*date,*countryFrom,*countryTo,*virusName,*t1,*t2,
	*inputBuffer = malloc(sizeof(char) * maxInputStingSize)

    // As you might have seen on the readme, there are two ways in which the program can terminate
    // The dafault is the "violent", as described in the assignment, all the child processes are killed once the exit is given
    // The "restfull" way tells the children to free their memory terminate (restfullyExit command), more on readme
    ,exitMethod = VIOLENTLY;//

    while( execution == 1 ){// The programs main and command execution starts and ends in this loop
		printf("\x1B[32m>/\x1B[37m");// '>/' is printed in green (all the other characters are ansi color codes)
		getline(&inputBuffer,&maxInputStingSize,stdin);//Reading user input

        //Checking if any signals were recieved from the parrent
        if(SigChld_Flag > 0){
            printf("I have recieved a child sig");//delete
            //This functions check wich processes have terminated, and returns the number of processes it "revived"
            SigChld_Flag -= checkMonitorsExistance();
            printf(" (%d left)\n",SigChld_Flag);//delete
            //Clearing the input string, and the input buffer from errors occured from the signal arival
            memset(inputBuffer,'\0',maxInputStingSize);
            clearerr(stdin);
            continue;
        }
        if(SigIntQuit_Flag>0){
            printf("I have recieved a int/quit sig");//delete
            SigIntQuit_Flag=0;
            printf(" (%d left)\n",SigChld_Flag);//delete
            execution=0;//The parrent will terminate "violently"
            clearerr(stdin);
            break;
        }

		command = strtok(inputBuffer, " ");// Getting first argument of user input, the command
		
		/*As you might have seen in the readme, it does not matter if commands start with '/' or not
		  If a command starts with '/' it is valid, we simply remove the '/' for our facility by
		  shifting all the characters of the string execpt the first one (the '/')*/
		if(command[0]=='/') memmove(command,command+sizeof(char),strlen(command));

		//The structure of if else statements executes the right command at each loop
		if( strcmp(command,"travelRequest") == 0 ){

			// The rest string on the input line are the arguments for the command
			citizenID = strtok(NULL, " ");
            date = strtok(NULL, " ");
            countryFrom = strtok(NULL, " ");
            countryTo = strtok(NULL, " ");
            virusName = strtok(NULL, " ");

			// Checking the arguments needed for this command
			if(citizenID != NULL && date != NULL && checkIfValidDate(date) == 1 && countryFrom != NULL && countryTo != NULL && virusName !=NULL){
				strtok(virusName, "\n");//The virusName almost centerly has the \n character
				// Calling the function of the controler which impliments the commands functionality
				if( travelRequest(citizenID,date,countryFrom,countryTo,virusName) !=0 )
                    printf("COMMAND ERROR\n");
			}else{
				printf("ERROR: ONE OR BOTH OF THE citizenID, date, countryFrom, countryTo, virusName  WAS MISSTYPED OR NOT GIVEN AT ALL!\n");
			}

		}else if( strcmp(command,"travelStats") == 0 ){
            virusName = strtok(NULL, " ");
            t1 = strtok(NULL, " ");
            t2 = strtok(NULL, " ");
            country = strtok(NULL, " ");

            //The last argument has the '\n' char which is removed from the string it is in
            if(country !=NULL)
                strtok(country, "\n");
            else
                strtok(t2, "\n");

            if(virusName != NULL && t1 != NULL && t2 != NULL){
				// Calling the function of the controler which impliments the commands functionality
				travelStats(virusName,t1,t2,country);
			}else{
				printf("ERROR: ONE OR MORE OF THE ARGUMENTS WAS MISSTYPED OR NOT GIVEN AT ALL!\n");
			}

        }else if( strcmp(command,"addVaccinationRecords") == 0){
            country = strtok(NULL, " ");

            if(country != NULL){
				strtok(country, "\n");// Removing '\n' from argument
				// Calling the function of the controler which impliments the commands functionality
				if( addVaccinationRecords(country,input_dir,numMonitors, bloomSize) !=0 )
                    printf("COMMAND ERROR\n");
			}else{
				printf("ERROR: ONE country ARGUMENT WAS MISSTYPED OR NOT GIVEN AT ALL!\n");
			}
        }else if( strcmp(command,"searchVaccinationStatus") == 0){
            citizenID = strtok(NULL, " ");

            if(citizenID != NULL){
				strtok(citizenID, "\n");

				if( searchVaccinationStatus(citizenID,numMonitors) !=0 )
                    printf("COMMAND ERROR\n");
			}else{
				printf("ERROR: ONE country ARGUMENT WAS MISSTYPED OR NOT GIVEN AT ALL!\n");
			}
        }else if( strncmp(command,"printPids",9) == 0 ){
			printPids(); // Is an extra not requested command which prints the pids of all the processes (usefull for signals)
		}else if( strncmp(command,"exit",4) == 0 ){
			//The program is terminated the default/"violent" way by breaking out of the execution loop
			execution =  0;
		}else if( strncmp(command,"restfullyExit",4) == 0 ){
			//The program is terminated the "restfull" way by breaking out of the execution loop and changing the exiy method
            exitMethod = RESTFULLY;
			execution =  0;
		}else{
			printf("Unvalid command (%s)!\n",command);
		}
	}
    free(inputBuffer);

    terminateTravelMonitor(numMonitors,exitMethod);
    return 0;
}
