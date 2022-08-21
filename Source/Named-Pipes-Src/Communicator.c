/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Utilities.h"

// The permisions which will be set on the fifo files
#define FIFO_PERMS 0777

// Each communicator consists of
typedef struct Communicator {
    char *name; // A name, which will be part of the filenames of the fifos in name_1 name_2 format
    char *writeFifoFile; // The filename of the writing end fifo (name_1 or name_2)
    int writeFifoFD; // The file discriptor of the above
    char *readFifoFile; // The filename of the reading end fifo
    int readFifoFD; // The file discriptor of the above
    unsigned int pipeBufferSize; // The size of the fifo pipe described in the assignment
} Communicator;

int getReadFifoFd(Communicator *c){
    return c->readFifoFD;
}

// Given the size of the buffer and the size of the message, it calculates in how many parts the file has to be sent
unsigned int calculatesNumOfPackets(unsigned int pipeBufferSize, unsigned int messageSize){
    return (messageSize/pipeBufferSize) + (messageSize%pipeBufferSize != 0);
}

// A wrapper for the read() syscall which handles a signal interupt error
ssize_t readWrapper(int fd, void *buf, size_t count){
    ssize_t result = read(fd, buf, count);
    if( result == -1){
        //Since this procces will receive singals, those singals might happen to be received when the process executes read()...
        //...leading to unexpected termination of the syscall with a return value of -1
        //In order to control the behavior of the program in the above case, errno is used to check the cause of the termination
        if(errno == EINTR){//If the cause was EINTR, wich according to the man means "The call was interrupted by a signal..."
            errno = 0;//We know a signal was received while we were executing poll and lead to an error, so re-initialize the errno value
            return -2;//And return the right error value
        }else{
            printf("read from communicator error\n");//If the error in read() was not caused by a signal
            return -3;//Something really unexpected happend
        }
    }
    return result;
}

/* This function is used in two ways:
 1. Before the monitor is aware of the buffer size, we do not now the exact size of it
    so the first message (an unsigned int which is the buffer size) is sent in chunks of chars (1 byte).
    The above happends when the argument pipeBufferSize=0
 2. The reciever must be aware of the message length of a message before it starts recieving it,
    if pipeBufferSize!=0 the function sends the unsigned int length of a message in chunks of pipeBufferSize
*/
void sendInitialBuffSize(unsigned int num,int writeFifoFD, unsigned int pipeBufferSize){
    unsigned char *intCharArrey = unsignedIntToChars(num); // Converting unsigned int to chars

    if(pipeBufferSize == 0){
        for(int i=0; i<4; i++){
            write(writeFifoFD,&(intCharArrey[i]),sizeof(char));
        }
    }else{
        unsigned int numOfPackets = calculatesNumOfPackets(pipeBufferSize,sizeof(char)*sizeof(unsigned int)), bytesWriten=0;
        int i;
        if(numOfPackets==1){
            // If the pipe can fit the entire message, the message is sent at once
            write(writeFifoFD,intCharArrey,sizeof(char)*sizeof(unsigned int));
        }else{
            // Otherwise the message is sent in chunks of pipeBufferSize
            for(i=0; i<numOfPackets-1; i++){
                bytesWriten += write(writeFifoFD,intCharArrey + i*pipeBufferSize,pipeBufferSize);
            }
            // Expect the last message which is at most pipeBufferSize
            write(writeFifoFD,intCharArrey + bytesWriten,sizeof(char)*sizeof(unsigned int)-bytesWriten);
        }
    }

    free(intCharArrey);
}

/* The above is the complementary function of the above, 
   it recieves an unsigned int in chunks of chars (1 byte) if pipeBufferSize=0
   and in pipeBufferSize chunks if pipeBufferSize!=0
*/
unsigned int recieveInitialBuffSize(int readFifoFD, unsigned int pipeBufferSize){
    unsigned int result;
    unsigned char *intCharArrey = malloc(sizeof(char)*sizeof(unsigned int));
    ssize_t readResult;

    if(pipeBufferSize == 0){
        for(int i=0; i<sizeof(char)*sizeof(unsigned int); i++){
           readResult = readWrapper(readFifoFD,&(intCharArrey[i]),sizeof(char));
           if(readResult < 0 ) {free(intCharArrey); return readResult;}
        }
    }else{
        unsigned int numOfPackets = calculatesNumOfPackets(pipeBufferSize,sizeof(char)*sizeof(unsigned int)), bytesRead=0;
        int i;
        if(numOfPackets==1){
        // As in sendMessage If the pipe can fit the entire message, the message is recieved at once
        readResult = readWrapper(readFifoFD,intCharArrey,sizeof(char)*sizeof(unsigned int));
        if(readResult < 0 ) {free(intCharArrey); return readResult;}
        }else{
            // Othersie the message is recieved in pipeBufferSize chunks
            for(i=0; i<numOfPackets-1; i++){
                readResult = readWrapper(readFifoFD,intCharArrey + i*pipeBufferSize,pipeBufferSize);
                if(readResult < 0 ) {free(intCharArrey); return readResult;}
                bytesRead += readResult;
            }
            readResult = readWrapper(readFifoFD,intCharArrey + bytesRead, sizeof(char)*sizeof(unsigned int) - bytesRead);
            if(readResult < 0 ) {free(intCharArrey); return readResult;}
        }
    }

    result = charsToUnsignedInt(intCharArrey);
    free(intCharArrey);
    return result;
}

// Creates all the files necesery for the a communicator named "name"
int createCommunicator(char* name){

    char *FifoName1=NULL,*FifoName2=NULL;
    // Creating the filenames of the two fifos
    myStringCat(&FifoName1,name,"_1");
    myStringCat(&FifoName2,name,"_2");

    // Creating the fifos
    if(mkfifo(FifoName1,FIFO_PERMS)==-1 || mkfifo(FifoName2,FIFO_PERMS)==-1){
        if(errno != EEXIST){
            // An actual error occured
            perror("mkfifo()"); 
            free(FifoName1); free(FifoName2);
            return -1;
        }else{
            perror("mkfifo(): Fifo exists!"); // If the fifo exists that does lead to an error, but a warning is printed
            free(FifoName1); free(FifoName2);
            return 1;
        }
    }

    free(FifoName1); free(FifoName2);
    return 0;
}

// Opens the communicator for a parrent/travel monitor
Communicator* openParrentCommunicator(char* name, unsigned int pipeBufferSize){
    Communicator* c = malloc( sizeof(Communicator) );
    if( c==NULL ) { perror("Communicator malloc()"); return NULL; }
    // Setting the viariables of the Communicator
    c->name = strdup(name);
    c->pipeBufferSize = pipeBufferSize;

    // Opening write fifo
    c->writeFifoFile = NULL;
    myStringCat(&(c->writeFifoFile),c->name,"_1");
    c->writeFifoFD = open(c->writeFifoFile,O_WRONLY);
    if(c->writeFifoFD==-1) {perror("Communicator open()"); free(c); return NULL;}

    // Opening read fifo
    c->readFifoFile = NULL;
    myStringCat(&(c->readFifoFile),c->name,"_2");
    c->readFifoFD = open(c->readFifoFile,O_RDONLY);
    if(c->readFifoFD==-1) {perror("Communicator open()"); free(c); return NULL;}

    // Sending the pipe size
    sendInitialBuffSize(c->pipeBufferSize,c->writeFifoFD,0);

    return c;
}

// Opens communicator for child/monitor
Communicator* openMonitorCommunicator(char* name){
    Communicator* c = malloc( sizeof(Communicator) );
    if( c==NULL ) { perror("Communicator malloc()"); return NULL; }

    // Setting the viariables of the Communicator
    c->name = strdup(name);

    // Opening read fifo
    c->readFifoFile = NULL;
    myStringCat(&(c->readFifoFile),c->name,"_1");
    while(access(c->readFifoFile,F_OK) != 0 && errno != ENOENT ){usleep(500000);}//bad idea
    c->readFifoFD = open(c->readFifoFile,O_RDONLY);
    if(c->readFifoFD==-1) {perror("Communicator open()"); free(c); return NULL;}

    // Opening write fifo
    c->writeFifoFile = NULL;
    myStringCat(&(c->writeFifoFile),c->name,"_2");
    while(access(c->readFifoFile,F_OK) != 0 && errno != ENOENT ){usleep(500000);}//bad idea
    c->writeFifoFD = open(c->writeFifoFile,O_WRONLY);
    if(c->writeFifoFD==-1) {perror("Communicator open()"); free(c); return NULL;}

    // Recieve the pipe size
    c->pipeBufferSize = recieveInitialBuffSize(c->readFifoFD,0);

    return c;
}

// Simply closes the communicator files, and frees the memory the structure had allocated (called from monitor)
int closeCommunicator(Communicator* c){
    close(c->readFifoFD);
    free(c->readFifoFile);

    close(c->writeFifoFD);
    free(c->writeFifoFile);

    free(c->name);

    free(c);

    return 0;
}

// Deletes the communicator files
int destroyCommunicator(char *name){
    if( name==NULL ) return -2;

    char *FifoName1=NULL,*FifoName2=NULL;
    myStringCat(&FifoName1,name,"_1");
    myStringCat(&FifoName2,name,"_2");

    if ( remove(FifoName1)==-1 || remove(FifoName2)==-1 ){
        perror("Communicator remove()"); 
        free(FifoName1); free(FifoName2);
        return -1;
    }

    free(FifoName1); free(FifoName2);
    return 0;

}

// Closes and deletes the communicator files, also frees the memory the structure had allocated (called from travel monitor)
int closeAndDestroyCommunicator(Communicator* c){
    char* name = strdup(c->name);
    if( closeCommunicator(c) !=0 ) return -1;
    if( destroyCommunicator(name) != 0 ) return -2;
    free(name);
    return 0;
}

// Sends a messsage using the given communicator
int sendMessage(Communicator* c,const void* message, unsigned int messageSize){
    unsigned int i,numOfPackets,bytesWriten=0;

    // Informing the reciever about the length of the message
    sendInitialBuffSize(messageSize,c->writeFifoFD,c->pipeBufferSize);

    numOfPackets = calculatesNumOfPackets(c->pipeBufferSize,messageSize);
    if(numOfPackets==1){
        // If the pipe can fit the entire message, the message is sent at once
        write(c->writeFifoFD,message,messageSize);
    }else{
        // Otherwise the message is sent in chunks of pipeBufferSize
        for(i=0; i<numOfPackets-1; i++){
            bytesWriten += write(c->writeFifoFD,message + i*(c->pipeBufferSize),c->pipeBufferSize);
        }
        // Expect the last message which is at most pipeBufferSize
        write(c->writeFifoFD,message + bytesWriten,messageSize-bytesWriten);
    }

    return 0;
}

// Recieves a messsage in the given communicator
int recieveMessage(Communicator* c, void* message, unsigned int *maxMessageSize){
    unsigned int i,numOfPackets, bytesRead=0;
    ssize_t readResult;

    memset(message,0,*maxMessageSize); // Clearing the given buffer

    // Getting the length of the message in bytes 
    int messageSize = recieveInitialBuffSize(c->readFifoFD,c->pipeBufferSize);
    if(messageSize<0){
        if(messageSize == -2)
            return -2; //Read interupted from a signal
        else
            return -3; //Somthing really unexpected happened
    }

    // Checking if the message can fit in the given buffer, otherwise a warning message is printed
    int ret = *maxMessageSize < messageSize;
    if(ret == 1){
        printf("%u: %d (given) is smaller than %d (needed)",getpid(),*maxMessageSize,messageSize);
    }
    *maxMessageSize = messageSize;

    numOfPackets = calculatesNumOfPackets(c->pipeBufferSize,messageSize);
    if(numOfPackets==1){
        // As in sendMessage If the pipe can fit the entire message, the message is recieved at once
        readResult = readWrapper(c->readFifoFD,message,messageSize);
        if(readResult < 0 ) return readResult;
    }else{
        // Othersie the message is recieved in pipeBufferSize chunks
        for(i=0; i<numOfPackets-1; i++){
            readResult = readWrapper(c->readFifoFD,message + i*(c->pipeBufferSize),c->pipeBufferSize);
            if(readResult < 0 ) return readResult;
            bytesRead += readResult;
        }
        readResult = readWrapper(c->readFifoFD,message + bytesRead,messageSize-bytesRead);
        if(readResult < 0 ) return readResult;
    }

    return ret;
}
