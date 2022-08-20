typedef struct Communicator Communicator;

int getCommunicationFd(Communicator *c);

Communicator* initClientCommunicator(int *port, unsigned int pipeBufferSize);
int closeClientCommunicator(Communicator* c);

Communicator* initServerCommunicator(int port, unsigned int socketBufferSize);
int closeServerCommunicator(Communicator* c);

int sendMessage(Communicator* c,const void* message, unsigned int messageSize);
int recieveMessage(Communicator* c, void* message, unsigned int *messageSize);