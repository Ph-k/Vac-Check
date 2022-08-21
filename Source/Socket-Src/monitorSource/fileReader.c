/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "BloomFilter.h"
#include "HashTable.h"
#include "monitorController.h"
#include "Utilities.h"

// Given a file with initialization data, the function reads it, and inserts the valid records, printing any error messages
int citizensFileReader(char* filename, pthread_mutex_t *mutexes){
	FILE *fp = fopen(filename,"r"); //Opening file for reading only
	
	if (fp == NULL) return -1; //If file does not exist error code is returned
	
	char Id[50], lastName[50], firstName[50], country[50], virusName[50], vaccinatedString[4], date[30];

	unsigned int age,inconsistencies=0;
	int temp;
	char vaccinated;
	while( fscanf(fp, "%s", Id) != EOF ){ //While we have not reached the end of file (reading the citizen ID which should be the first information given)

		fscanf(fp, "%s", firstName); //After the ID the first name of the citizen should be in the text file

		fscanf(fp, "%s", lastName); //Accordingly the rest information of the studen is read

		fscanf(fp, "%s", country);

		fscanf(fp, "%d", &age);

		fscanf(fp, "%s", virusName);
		
		fscanf(fp, "%s", vaccinatedString);

		memset(date,'\0',10); // Emptying the date string before checking if we will read its value

		if(strcmp(vaccinatedString,"YES")==0){// If the citizen is vaccinated for the given virus
			vaccinated = 1;
			fscanf(fp, "%[^\n]", date);// Reading all the characters that are left in this line of the file
			removeCharFromString(date,' ');// Removing white spaces
			if(checkIfValidDate(date)!=1){// If somthing other than a date was given in the dates place
				printf("FORMAT ERROR IN RECORD %s %s %s %s %d %s %s %s\n",Id,firstName,lastName,country,age,virusName,vaccinatedString,date);
				continue;// An error is printed and the record will not be inserted
			}
		}else{// The citizen is not vaccinated foe the virus case
			fscanf(fp, "%[^\n]", date);// Reading all the characters that are left in this line of the file
			if(checkEmptyString(date)==0){// Nothing else should be given after the NO in the line, especialy not a date
				// If somthing was given after the NO, an error is printed
				printf("ERROR IN RECORD %s %s %s %s %d %s %s",Id,firstName,lastName,country,age,virusName,vaccinatedString);
				removeCharFromString(date,' ');
				if(checkIfValidDate(date)==1){// If a date was given, a date error is also printed
					printf(" (a date was given!)\n");
				}else printf("\n");
				continue;// Since error occured, the record will not be inserted
			}
			vaccinated = 0;
		}

		// If the record was valid, the new citizen is inserted (more on what silentInsert does, on controller.c)
		temp = silentInsert(Id,lastName,firstName,country,age,virusName,vaccinated,date,mutexes);
		inconsistencies = 0;
		if( temp == -3 ){// If the insertion was not succesfull, the record had an error, -3 means the record exists
			if(inconsistencies<10){
				if(date!=NULL)
					printf("ERROR IN RECORD %s %s %s %s %d %s %s %s: CITIZEN WITH THIS ID EXISTS. BUT WITH DIFFERENT INFO (NAME, COUNTRY, AGE)!\n",
					Id,firstName,lastName,country,age,virusName,vaccinatedString,date);
				else
					printf("ERROR IN RECORD %s %s %s %s %d %s %s: CITIZEN WITH THIS ID EXISTS. BUT WITH DIFFERENT INFO (NAME, COUNTRY, AGE)!\n",
					Id,firstName,lastName,country,age,virusName,vaccinatedString);
			}else if(inconsistencies==10){
				//In order to not fill the terminal with error messages from a bad file, all the inconsistant records are counted, but only the first 100 are printed
				printf("MORE THAN 100 PROBLEMATIC RECORDS WERE FOUND IN %s,\nALL OTHER PROBLEMATIC RECORDS WILL ONLY BE COUNTED AND NOT PRINTED :-)\n",filename);
			}
			inconsistencies++;
		}else if(temp == -1){// -1 means that this exact citizen has already information saved about this particular virus
			if(inconsistencies<10){
				if(date!=NULL)
					printf("ERROR IN RECORD %s %s %s %s %d %s %s %s : THERE HAS ALREADY BEEN A RECORD FOR THIS CITIZEN AND VIRUS!\n",
					Id,firstName,lastName,country,age,virusName,vaccinatedString,date);
				else
					printf("ERROR IN RECORD %s %s %s %s %d %s %s  : THERE HAS ALREADY BEEN A RECORD FOR THIS CITIZEN AND VIRUS!\n",
					Id,firstName,lastName,country,age,virusName,vaccinatedString);
			}else if(inconsistencies==10){
				printf("MORE THAN 10 PROBLEMATIC RECORDS WERE FOUND IN %s,\nALL OTHER PROBLEMATIC RECORDS WILL ONLY BE COUNTED AND NOT PRINTED :-).\n",filename);
			}
			inconsistencies++;
		}else if( temp!=0 ) printf("General silent insert error (%d)!\n",temp);

	}

	fclose(fp); //Closing file
	return 0;
}
