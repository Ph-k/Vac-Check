// Given the process id and a message. It is writen to the log file named log.xxx where xxx the pid
int writeToLog(char* string);

//Used to print the counties in the logfile while traversing a string dictonary by calling writeToLog
void dummyCountyLogWritter(void* countryString, void* nothing);