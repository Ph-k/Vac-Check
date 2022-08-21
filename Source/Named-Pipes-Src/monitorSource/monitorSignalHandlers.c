/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
#include <unistd.h>
#include <signal.h>

// These signal flaga are of global scope (see extern in .h)
// And allow the monitor to see which singals have been recieved
char SigUsr1_Flag=0,
    SigIntQuit_Flag=0;

// The handler of the usr1, "raises" the usr1 flag when a usr1 is recieved
void usr1Handler(int sigNum){
    SigUsr1_Flag++;
}

// Similary the handler of the int and quit signals, "raises" the int/quit flag
void sigIntQuitHandler(int signum){
    SigIntQuit_Flag++;
}

static struct sigaction sa1 = {0},sa2 = {0};

// Sets all the signal handlers needed for the monitor using sigaction
void setSignalHandlers(){
    sa1.sa_handler = usr1Handler;
    sigaction(SIGUSR1, &sa1, NULL);

    sa2.sa_handler = sigIntQuitHandler;
    sigaction(SIGQUIT, &sa2, NULL);
    sigaction(SIGINT, &sa2, NULL);
}