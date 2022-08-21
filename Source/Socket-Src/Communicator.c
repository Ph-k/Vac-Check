/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "Utilities.h"


// A macro to perror the error message and return
#define ERROR_RETURN(mess,e) ({\
                                perror(mess);\
                                return e;\
                              })

// Each communicator consists of
typedef struct Communicator {
    int socketFD, communicationFD; // The file descriptor of the socket and the one in whicih the communication occurs 
    struct sockaddr_in serverAddres; // The ip adress
    unsigned int pipeBufferSize; // The size of the socket size as described in the assignment
} Communicator;

int getCommunicationFd(Communicator *c){
    return c->communicationFD;
}

// Given the size of the buffer and the size of the message, it calculates in how many parts the file has to be sent
unsigned int calculatesNumOfPackets(unsigned int pipeBufferSize, unsigned int messageSize){
    return (messageSize/pipeBufferSize) + (messageSize%pipeBufferSize != 0);
}

/* The reciever must be aware of the message length of a message before it starts recieving it,
    the function sends the unsigned int length of a message in chunks of pipeBufferSize of char type
*/
void sendInitialBuffSize(unsigned int num,int writeFifoFD, unsigned int pipeBufferSize){
    unsigned char *intCharArrey = unsignedIntToChars(num); // Converting unsigned int to chars

    unsigned int numOfPackets = calculatesNumOfPackets(pipeBufferSize,sizeof(char)*sizeof(unsigned int)), bytesWriten=0;
    int i;
    if(numOfPackets==1){
        // If the pipe can fit the entire message, the message is sent at once
        send(writeFifoFD,intCharArrey,sizeof(char)*sizeof(unsigned int),0);
    }else{
        // Otherwise the message is sent in chunks of pipeBufferSize
        for(i=0; i<numOfPackets-1; i++){
            bytesWriten += send(writeFifoFD,intCharArrey + i*pipeBufferSize,pipeBufferSize,0);
        }
        // Expect the last message which is at most pipeBufferSize
        send(writeFifoFD,intCharArrey + bytesWriten,sizeof(char)*sizeof(unsigned int)-bytesWriten,0);
    }

    free(intCharArrey);
}

/* The above is the complementary function of the above, 
   it recieves an unsigned int in chunks of chars, based on the bufferSize
*/
unsigned int recieveInitialBuffSize(int readFifoFD, unsigned int pipeBufferSize){
    unsigned int result;
    unsigned char *intCharArrey = malloc(sizeof(char)*sizeof(unsigned int));
    ssize_t readResult;

    unsigned int numOfPackets = calculatesNumOfPackets(pipeBufferSize,sizeof(char)*sizeof(unsigned int)), bytesRead=0;
    int i;
    if(numOfPackets==1){
        // As in sendMessage If the pipe can fit the entire message, the message is recieved at once
        readResult = recv(readFifoFD,intCharArrey,sizeof(char)*sizeof(unsigned int),MSG_WAITALL);
        if(readResult < 0 ) {free(intCharArrey); return readResult;}
    }else{
        // Othersie the message is recieved in pipeBufferSize chunks
        for(i=0; i<numOfPackets-1; i++){
            readResult = recv(readFifoFD,intCharArrey + i*pipeBufferSize,pipeBufferSize,MSG_WAITALL);
            if(readResult < 0 ) {free(intCharArrey); return readResult;}
            bytesRead += readResult;
        }
        readResult = recv(readFifoFD,intCharArrey + bytesRead, sizeof(char)*sizeof(unsigned int) - bytesRead,MSG_WAITALL);
        if(readResult < 0 ) {free(intCharArrey); return readResult;}
    }

    result = charsToUnsignedInt(intCharArrey);
    free(intCharArrey);
    return result;
}

// It simply finds the hostname and it's lenght, while allocatinf any needed memory, a "fancy" wrapper for the gethostname()
int getHostNameWrapper(char **hostname, int *length){
	*hostname = NULL;
	*length=10;
	char flag = 1;
	while(flag == 1){
		*hostname = malloc(sizeof(char)*(*length));
		if(*hostname == NULL) return -2;
		memset(*hostname,0,sizeof(char)*(*length));

		if( gethostname(*hostname,*length) == 0) { flag = 0; break; }

		free(*hostname);
		if( errno == ENAMETOOLONG ){
            // If the allocated memory was not enough, we free the allocated memory and allocate more on the next itteration
			(*length)++;
		}else return -1;
	}
	return 0;
}

// Opens the communicator for a parrent/travel monitor/client
Communicator* initClientCommunicator(int *port, unsigned int pipeBufferSize){
    Communicator* c = malloc( sizeof(Communicator) );
    if( c==NULL ) { ERROR_RETURN("Communicator malloc()" , NULL); }
	c->pipeBufferSize = pipeBufferSize;

    c->socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if( c->socketFD < 0 ) { ERROR_RETURN("Communicator socket()" , NULL); }

    // Initilizing sockaddr_in struct
    memset(&(c->serverAddres),0,sizeof(struct sockaddr_in));
    c->serverAddres.sin_family = AF_INET;
    c->serverAddres.sin_port = htons(*port);

    char *hostName;
    int hostNameLenght;
    getHostNameWrapper(&hostName, &hostNameLenght);

    struct hostent *rem;
    rem = gethostbyname(hostName);
    if (rem  == NULL) {free(hostName); ERROR_RETURN("Communicator gethostbyname()", NULL); }
    memcpy(&(c->serverAddres.sin_addr), rem->h_addr, rem->h_length);

    free(hostName);

    // Connecting to the socket
    while( connect(c->socketFD, (struct sockaddr*)&(c->serverAddres), sizeof(struct sockaddr_in) ) < 0 ){
		if(errno == ECONNREFUSED){
            // retrying if connection happended to get refuresed
			continue;
		}else{
            // Something really unexpected happend
			ERROR_RETURN("Communicator connect()",NULL); 
		}
	}

    // For the client the communication FD is also the socket FD from socket()
    c->communicationFD = c->socketFD;

    return c;
}

// Opens communicator for child/monitor/server
Communicator* initServerCommunicator(int port, unsigned int socketBufferSize){
    Communicator* c = malloc( sizeof(Communicator) );
    if( c==NULL ) { ERROR_RETURN("Communicator malloc()" , NULL); }
    c->pipeBufferSize = socketBufferSize;

    c->socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if( c->socketFD < 0 ) { ERROR_RETURN("Communicator socket()" , NULL); }

    // Initilizing sockaddr_in struct
    memset(&(c->serverAddres),0,sizeof(struct sockaddr_in));
    c->serverAddres.sin_family = AF_INET;
    c->serverAddres.sin_port = htons(port);

    // In order to avoid bind failure due to the socket beeing in a TIME_WAIT state from a previous execution
    // We use setsockopt() as noted from the instructors
    int optval = 1;
	setsockopt(c->socketFD, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int) );

    // Binding and listening for a connection from the client/parrent process
	if( bind( c->socketFD, (struct sockaddr*)&(c->serverAddres), sizeof(struct sockaddr_in) ) < 0 ){ ERROR_RETURN("Communicator bind()" , NULL); }
    if( listen(c->socketFD,1) < 0 ){ ERROR_RETURN("Communicator listen()" , NULL); }

    // For the server, the communication FD is the one that accept() returned
    c->communicationFD = accept(c->socketFD,(struct sockaddr*)NULL,NULL);

    return c;
}

// Simply closes the communication fd, and frees the memory the structure had allocated (called from travel monitor/client)
int closeClientCommunicator(Communicator* c){
    close(c->socketFD); // The client only has one FD
    free(c);
    return 0;
}

// Simply closes the communicator fd's, and frees the memory the structure had allocated (called from monitor/server)
int closeServerCommunicator(Communicator* c){
    // The server had two fd's
    close(c->communicationFD);
    close(c->socketFD);
    free(c);
    return 0;
}

// Sends a messsage using the given communicator
int sendMessage(Communicator* c,const void* message, unsigned int messageSize){
    unsigned int i,numOfPackets,bytesWriten=0;
    // Informing the reciever about the length of the message
    sendInitialBuffSize(messageSize,c->communicationFD,c->pipeBufferSize);

    numOfPackets = calculatesNumOfPackets(c->pipeBufferSize,messageSize);
    if(numOfPackets==1){
        // If the pipe can fit the entire message, the message is sent at once
        send(c->communicationFD,message,messageSize,0);
    }else{
        // Otherwise the message is sent in chunks of pipeBufferSize
        for(i=0; i<numOfPackets-1; i++){
            bytesWriten += send(c->communicationFD,message + i*(c->pipeBufferSize),c->pipeBufferSize,0);
        }
        // Expect the last message which is at most pipeBufferSize
        send(c->communicationFD,message + bytesWriten,messageSize-bytesWriten,0);
    }

    return 0;
}

// Recieves a messsage in the given communicator
int recieveMessage(Communicator* c, void* message, unsigned int *maxMessageSize){
    unsigned int i,numOfPackets, bytesRead=0;
    ssize_t readResult;

    memset(message,0,*maxMessageSize); // Clearing the given buffer

    // Getting the length of the message in bytes 
    int messageSize = recieveInitialBuffSize(c->communicationFD,c->pipeBufferSize);
    if(messageSize<0){
        if(messageSize == -2)
            return -2; //Read interupted from a signal (from previous project, there is no sigal handling)
        else
            return -3; //Something really unexpected happened
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
        readResult = recv(c->communicationFD,message,messageSize,MSG_WAITALL);
        if(readResult < 0 ) return readResult;
    }else{
        // Othersie the message is recieved in pipeBufferSize chunks
        for(i=0; i<numOfPackets-1; i++){
            readResult = recv(c->communicationFD,message + i*(c->pipeBufferSize),c->pipeBufferSize,MSG_WAITALL);
            if(readResult < 0 ) return readResult;
            bytesRead += readResult;
        }
        readResult = recv(c->communicationFD,message + bytesRead,messageSize-bytesRead,MSG_WAITALL);
        if(readResult < 0 ) return readResult;
    }

    return ret;
}
