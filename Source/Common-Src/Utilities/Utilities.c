/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

/*Given an unisgned int (ussualy 4 bytes long) it places it's bits to an 
arrey of chars (each char is ussualy 1 byte long, the smallest variable available)*/
unsigned char* unsignedIntToChars(unsigned int num){
    //The length of the resulted arrey will be char size times the size of the unsigned int
    unsigned int numArreyLen = sizeof(char)*sizeof(unsigned int);
    unsigned char *numArrey=malloc(numArreyLen);

    //Creating a '1' mask char bits long (since CHAR_BIT = how many bits in a char)
    int charSizeMask=0;
    for(int i=0; i<CHAR_BIT; i++){
        charSizeMask |= charSizeMask << 1;
        charSizeMask |= 1;
    }

    //Spliting the unsigned int to memory spaces of char size...
    for(int i=0; i<numArreyLen; i++){
        //...using the mask and logigal shifts according to possition of the bits we are splitting
        numArrey[i] = (num >> i*CHAR_BIT) & charSizeMask;
    }
    
    return numArrey;
}

/*Given an char arrey of unsigned int size, which will contain the bits of an unsigned int,
it "converts it" to an unsigned int variable*/
unsigned int charsToUnsignedInt(unsigned char* numArrey){
    unsigned int result=0,numArreyLen = sizeof(char)*sizeof(unsigned int);

    //Placing the bits of the unsigned int in their correct possitions from the char arrey...
    for(int i=0; i<numArreyLen; i++){
        //Using again logical shifts and a logical OR to place the bits correctly 
        result |= numArrey[i] << i*CHAR_BIT;
    }
    
    return result;
}

//Given an unsigned int it returns the number of digits in it's decimal represetation
int digitsCount(unsigned int num){
	if(num==0) return 1; //0 has only one digit
	int count=0;
	while(num!=0){
        num=num/10;//Each division with 10 resulting to a non zero value...
        count++;//...means one more digit in the given number
    }
	return count;
}

// Converts the given int to a string, taking care of all the needed memory allocations
char* strIntDup(int val){
    //Calculating the needed memory size and allocating it
    int len = sizeof(char)*(digitsCount(val)+1);
    char* valString = malloc(len);
    // Initializing the new string
    memset(valString,0,len);
    // Placing the value of the int to the string using sprintf
    sprintf(valString,"%d",val);
    return valString;
}

// Converts the given unsigned int to a string, taking care of all the needed memory allocations
char* strUIntDup(unsigned int val){
    // Works as the above but sprintf converts an unsigned int
    int len = sizeof(char)*(digitsCount(val)+1);
    char* valString = malloc(len);
    memset(valString,0,len);
    sprintf(valString,"%u",val);
    return valString;
}

// Given a buffer and it's size, it checks if all its bytes are set to '0'
int checkIfNull(char *buff, int buffsize){
    char nullByte = 0;
    for(int i=0; i<buffsize; i++){
        // If only one byte is found to be !=0, then the buffer is considered not null
        if( nullByte != buff[i] ) return 0;
    }
    // Otherwise all the bytes were eqaul to 0, and the buffer is considered null
    return 1;
}

//Given a date string in the form of DD-MM-YYYY, it resturns the numerical d,m,y values
void extractY_D_M(const char* date, char *day,  char *month, int *year){
	// Duplicating the given date string, since it will be modified
	char *dateDup = strdup(date);
    // We need to preserve the value of the original pointer for free()
    char *dateDupTemp = dateDup;

	// Extracing day-year-month numbers from the string
	*day = atoi(strsep(&dateDupTemp,"-"));
	*month = atoi(strsep(&dateDupTemp,"-"));
    *year = atoi(strsep(&dateDupTemp,"-"));

	free(dateDup);
}

// Function to compare date strings. =0 => date1==date2 / >0 => date1>date2 / <0 => date1<date2
int dateCmp(char* Date_1, char* Date_2){
	
	// Extracing day-year-month numbers from the strings
	char day1, month1;
    int year1;
	extractY_D_M(Date_1,&day1,&month1,&year1);

    char day2, month2;
    int year2;
	extractY_D_M(Date_2,&day2,&month2,&year2);

	// The comparison is done
	// First by year (since then we do not need to check the month and date)
    if(year1>year2){
        return 3;
    }else if(year1<year2){
        return -3;
    }

	// Then by month
    if(month1>month2){
        return 2;
    }else if(month1<month2){
        return -2;
    }

	// And finally by date
    if(day1>day2){
        return 1;
    }else if(day1<day2){
        return -1;
    }

	// If none of the above was true, then the dates are the same
    return 0;
}

int dateDiffernceInMonths(char* Date_1, char* Date_2){
	// Extracing day-year-month numbers from the strings
	char day1, month1;
    int year1;
	extractY_D_M(Date_1,&day1,&month1,&year1);

    char day2, month2;
    int year2;
	extractY_D_M(Date_2,&day2,&month2,&year2);

    int tYear,tMonth,tDay;

    // Swapping the dates if date1 is after date2
    if( (year1>year2) || (year1==year2 && month1>month2) || (year1==year2 && month1==month2 && day1>day2) ){
        tYear = year1;
        tMonth = month1;
        tDay = day1;
        
        year1 = year2;
        month1 = month2;
        day1 = day2;
        
        year2 = tYear;
        month2 = tMonth;
        day2 = tDay;
    }

    int monthDiff;

    // Each year results in a 12-month differnce (if we do not have a full year diffenrce the month subtraction will make up)
    monthDiff = (year2 - year1) * 12;

    // The months differnce (negative results will make up for the above month difference in non-full year differances)
    monthDiff += month2 - month1;

    // If we are not on the same month, we also account for date differences if a full month has passed
    if (month2 != month1)
        monthDiff += (day2>=day1? 1 : 0);

    return monthDiff; // Retunring the result difference in months
}

void removeCharFromString(char* string,char c){
	int first=0,second=0;
	while( string[first]!='\0' ){// Traversing the whole string
		// Skipping all the unwanted characters, & overwriting them with wanted characters
	    while( string[first] == c ) first++;
	    string[second++]=string[first++];
	}
	string[second]='\0';// Placing the end of string at the right position
}

char* getCurrentDate(){
    char *date = malloc(sizeof(char)*11);// A date can have at most 11 characters (including '\0')
	memset(date,'\0',10);// Initialy the string is empty
	// Getting current time of the system
    time_t time_val = time(NULL);
    struct tm currentTime = *localtime(&time_val);
	// Placing it to the string
    sprintf(date,"%d-%d-%d",currentTime.tm_mday, currentTime.tm_mon + 1, currentTime.tm_year + 1900);
    return date;
}

int checkIfValidDate(const char* dateString){
	int length = strlen(dateString),i=0,j,day=0,month=0,year=0;

	if(length<5 || length>10) return 0; //A valid date lenght has min date is 1-1-1 and max 31-12-9999

	// Also, a valid date has exactly 3 numbers, seperated with '-'

    j=0; // Getting the first number (aka day)
	for(i=0; dateString[i]!='-' && i<length; i++)
        day = day*10 + dateString[i]-'0';
    if(i==j || day>31) return 0; // There are at most 31 days

    j=i+1; // Getting the first number (aka month)
    for(i=i+1; dateString[i]!='-' && i<length; i++)
        month = month*10 + dateString[i]-'0';
    if(i==j || month>12) return 0; // There are at most 12 months

    j=i+1; // Getting the first number (aka year)
    for(i=i+1; dateString[i]!='-' && i<length; i++)
        year = year*10 + dateString[i]-'0';
    if(i==j) return 0;

    return 1;
}

//My hash function, used in almost all hashtables
unsigned int voidStringHash(void* key,unsigned int size){ //Hash function
	char *k = (char*)key;
	unsigned int i, hash = 5381; //The hash is initialized as the lenght of the key logigal or with a prime number

	for (i=0; i<strlen(k); i++){
		//Each character in the key string characters are used along with some logical operations in order to create the hash
		hash ^= k[i] * 31;
		hash ^= hash << 7;
	}

	return hash%size; //(hash)mod(size) is used to not superpass the size of the hash table
}

// Checks if a string is emty by means of only consisting of whitespaces
int checkEmptyString(const char* string){
	int i=0;
	while( string[i]!='\0' ){
	    if( string[i] != ' ' ) return 0;
		i++;
	}
	return 1;
}

/*Given two strings, it combines them to a single one
taking care of all the memory allocation that might be needed*/
void myStringCat(char **concated,const char *string1,const char *string2){
	int i,j=0,t;

	//Taking care of memory allocation
	if(*concated==NULL){
		//allocating memorry, needed since given string points to NULL
		*concated = malloc( sizeof(char)*(strlen(string1)+strlen(string2)+2) );
		memset(*concated,0,strlen(string1)+strlen(string2)+2);
	}else if(strlen(*concated) < strlen(string1)+strlen(string2)+2 ){
		//allocating memorry, needed since allocated memory is not enough 
		free(*concated);
		*concated = malloc( sizeof(char)*(strlen(string1)+strlen(string2)+2) );
		memset(*concated,0,strlen(string1)+strlen(string2)+2);
	}else{
		//Allocating memory is enough, sipmly initialazing string
		memset(*concated,0,strlen(*concated));
	}

	//Copying first string
	t=strlen(string1);
	for(i=0; i<t; i++){
		(*concated)[j++] = string1[i];
	}

	//Copying the second string
	t=strlen(string2);
	for(i=0; i<t; i++){
		(*concated)[j++] = string2[i];
	}

	(*concated)[j] = '\0';//Placing end of string
}