// These global variables allow anyone who includes this file,
// and set the signal handlers. To know which signals have been recieved
extern char SigIntQuit_Flag;
extern char SigChld_Flag;

void setSignalHandlers();