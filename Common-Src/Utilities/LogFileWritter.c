#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //For getpid();

int pidDigitsCount(pid_t num){
    if(num==0) return 1; //0 has only one digit
    int count=0;
    while(num!=0){
        num=num/10;//Each division with 10 resulting to a non zero value,
        count++;//Mean one more digit in our number
    }
    return count;
}

// Given the process id and a message. It is writen to the log file named log.xxx where xxx the pid
int writeToLog(char* string){
    // Getting the process id in order for to be saved in the lof file name
    pid_t pid = getpid();

    // Creating/Opening file for appending
    char* fileName = malloc(sizeof(char)*(strlen("log.")+pidDigitsCount(pid)+1));
    sprintf(fileName,"log.%d",pid);
	FILE *fp = fopen(fileName, "a");
    free(fileName);
	if(fp==NULL) return -1;

  	//Writing message in the log file
	fprintf(fp,"%s\n",string);
	return fclose(fp);//Closing the log file, nothing else needs to be write here
}

//Used to print the counties in the logfile while traversing a string dictonary
void dummyCountyLogWritter(void* countryString, void* nothing){
    	writeToLog((char*)countryString);
}