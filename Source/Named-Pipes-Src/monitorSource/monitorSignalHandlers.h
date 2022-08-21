/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
// These global variables allow anyone who includes this file,
// and set the signal handlers. To know which signals have been recieved
extern char SigUsr1_Flag;
extern char SigIntQuit_Flag;

void setSignalHandlers();