#include <unistd.h>
#include <signal.h>

// These signal flaga are of global scope (see extern in .h)
// And allow the travelMonitor to see which singals have been recieved
char SigIntQuit_Flag=0;
char SigChld_Flag=0;

// The handler of the quit, "raises" the quit flag when a quit is recieved
void sigIntQuitHandler(int signum){
    SigIntQuit_Flag++;
}

// Similary the handler of the chld signal, "raises" the chld flag
void sigChldHandler(int signum){
    SigChld_Flag++;
}

static struct sigaction sa1 = {0}, sa2 = {0};

// Sets all the signal handlers needed for the travelMonitor using sigaction
void setSignalHandlers(){
    sa1.sa_handler = sigIntQuitHandler;
    sigaction(SIGINT, &sa1, NULL);
    sigaction(SIGQUIT, &sa1, NULL);

    sa2.sa_handler = sigChldHandler;
    sigaction(SIGCHLD, &sa2, NULL);
}