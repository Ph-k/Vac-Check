typedef struct Communicator Communicator;

int getReadFifoFd(Communicator *c);

int createCommunicator(char* name);
Communicator* openParrentCommunicator(char* name, unsigned int pipeBufferSize);
int closeAndDestroyCommunicator(Communicator* c);

Communicator* openMonitorCommunicator(char* name);
int closeCommunicator(Communicator* c);

int sendMessage(Communicator* c,const void* message, unsigned int messageSize);
int recieveMessage(Communicator* c, void* message, unsigned int *messageSize);