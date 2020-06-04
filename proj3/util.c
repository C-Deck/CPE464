#include "util.h"
#include "checksum.h"
#include "cpe464.h"
#include "gethostbyname.h"

#define MAXPDU 88

int UTIL_MODE = DEBUG_MODE;

// SERVER FUNCTIONS

int safeRecvfrom(int socketNum, void * buf, int len, int flags, struct sockaddr *srcAddr, int * addrLen)
{
	int returnValue = 0;
	if ((returnValue = recvfrom(socketNum, buf, (size_t) len, flags, srcAddr, (socklen_t *) addrLen)) < 0)
	{
		perror("recvfrom: ");
		exit(-1);
	}
	
	return returnValue;
}

int startServer(int portNumber)
{
	struct sockaddr_in6 server;
	int socketNum = 0;
	int serverAddrLen = 0;	
	
	// create the socket
	if ((socketNum = socket(AF_INET6,SOCK_DGRAM,0)) < 0)
	{
		perror("socket() call error");
		exit(-1);
	}
	
	// set up the socket
	server.sin6_family = AF_INET6;    		// internet (IPv6 or IPv4) family
	server.sin6_addr = in6addr_any ;  		// use any local IP address
	server.sin6_port = htons(portNumber);   // if 0 = os picks

	// bind the name (address) to a port
	if (bind(socketNum, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		perror("bind() call error");
		exit(-1);
	}

	/* Get the port number */
	serverAddrLen = sizeof(server);
	getsockname(socketNum,(struct sockaddr *) &server,  (socklen_t *) &serverAddrLen);
	printf("Server using Port #: %d\n", ntohs(server.sin6_port));

	return socketNum;
}

int connectServer(struct UDPConnection *udp, char * hostName, int portNumber)
{
	// currently only setup for IPv4 
	udp->socket = 0;
	char ipString[INET6_ADDRSTRLEN];
	uint8_t * ipAddress = NULL;
	
	// create the socket
	if ((udp->socket = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket() call error");
		exit(-1);
	}
  	 	
	if ((ipAddress = gethostbyname6(hostName, &(udp->server))) == NULL)
	{
      perror("gethostbyname6() call error");
		exit(-1);
	}
	
	udp->server.sin6_port = ntohs(portNumber);
	udp->server.sin6_family = AF_INET6;
	
	inet_ntop(AF_INET6, ipAddress, ipString, sizeof(ipString));
	printf("Server info - IP: %s Port: %d \n", ipString, portNumber);
		
	return udp->socket;
}

// PACKET FUNCTIONS

uint8_t * createPDU(uint32_t sequenceNumber, uint8_t flag, uint8_t *payload, int dataLen)
{
	static uint8_t pduBuffer[MAXPDU];

	if (UTIL_MODE == DEBUG_MODE) {
        printf("Creating the PDU with payload length %d\n", dataLen);
		printf("Payload: %s\n" (char*) payload);
    }

	// Build the PDU
	((uint32_t *) pduBuffer)[0] = htonl(sequenceNumber);
	pduBuffer[6] = flag;
	if (dataLen != 0) {
		memcpy(&pduBuffer[7], payload, dataLen);
	}

	printf("Old Checksum: %s\n", ((uint16_t *) pduBuffer)[2]);
    
    // Do checksum on pdu after payload has been copied
	((uint16_t *) pduBuffer)[2] = in_cksum((unsigned short *) pduBuffer, dataLen + 7);

	printf("New Checksum: %s\n", ((uint16_t *) pduBuffer)[2]);

	return pduBuffer;
}

void outputPDU(uint8_t * aPDU, int pduLength)
{
    uint16_t checksum = 0;
    uint8_t flag = 0;
    uint32_t sequenceNumber = 0;

    sequenceNumber = ntohl(((uint32_t *) aPDU)[0]);
    checksum = ((uint16_t *) aPDU)[2];
    flag = aPDU[6];

	printf("-----------------PDU-------------------\n");
    printf("Sequence Number: %d\n", sequenceNumber);
    printf("Checksum: %d\n", checksum);
    printf("Flag: %d\n", flag);
	printf("PDU Length: %d\n", pduLength);
	printf("Payload: %.*s\n", pduLength - 7, &(aPDU[7]));
	printf("---------------------------------------\n");
}

// Select function we got before but with specific size for int
int32_t selectCall(int32_t socketNumber, int32_t seconds, int32_t microseconds, int32_t timeIsNotNull)
{
    /* this function is written to only look at one socket  */

    // Returns 1 if socket is ready, 0 if socket is not ready  
    // set timeIsNotNull = TIME_IS_NOT_NULL when providing a time value
    int numReady = 0;
    fd_set fileDescriptorSet;
    struct timeval timeout;
    struct timeval * timeoutPtr;

	FD_ZERO(&fileDescriptorSet);
    FD_SET(socketNumber, &fileDescriptorSet);

    if (timeIsNotNull == TIME_IS_NOT_NULL) {
        timeout.tv_sec = seconds;
        timeout.tv_usec = microseconds;
        timeoutPtr = &timeout;
    }

    if ((numReady = select(socketNumber + 1, &fileDescriptorSet, NULL, NULL, timeoutPtr)) < 0) {
        perror("select");
        exit(-1);
    }

	if (UTIL_MODE == DEBUG_MODE) {
		//printf("Select call returned: %d\n", numReady);
	}

	// Will be either 0 (socket not ready) or 1 (socket is ready for read)
    return numReady;
}

int32_t recvCall(uint8_t *dataBuffer, uint32_t len, int32_t socket, UDPConnection *connection, uint8_t *flag, uint32_t *sequenceNumber)
{
    int dataLen = 0;
    uint8_t aPDU[MAX_BUFFER];
    uint32_t clientAddrLen = sizeof(struct sockaddr_in6);
    uint16_t checksum = 0;
	unsigned short checksumResult = 0;

    dataLen = recvfrom(socket, aPDU, len, 0, (struct sockaddr *) &(connection->server), &clientAddrLen);

    if (UTIL_MODE == DEBUG_MODE) {
        outputPDU(aPDU, dataLen);
    }

    *sequenceNumber = ntohl(*((uint32_t *) aPDU));
    *flag = aPDU[6];

    // Check if more than just the header
    if (dataLen > 7) {
        memcpy(dataBuffer, &(aPDU[7]), dataLen - 7);
    }

    memcpy(&checksum, &(aPDU[4]), 2);

    if ((checksumResult = in_cksum((unsigned short *) aPDU, dataLen)) != 0) {
		if (UTIL_MODE == DEBUG_MODE) {
			printf("Bad checksum: %d\n", checksumResult);
		}
        return RECV_ERROR;
    }

    return (dataLen - 7);
}


int32_t baseRecvCall(uint8_t *dataBuffer, uint32_t len, int32_t socket, UDPConnection *connection)
{
    // Empty variables for calling recvCall
	uint8_t flag;
	uint32_t sequenceNumber;

    return recvCall(dataBuffer, len, socket, connection, &flag, &sequenceNumber);
}

int32_t sendCall(uint8_t *dataBuffer, uint32_t dataLen, UDPConnection *connection, uint8_t flag, uint32_t sequenceNumber)
{
    uint32_t clientAddrLen = sizeof(struct sockaddr_in6);
    uint8_t * aPDU = NULL;
	int sendOutput = 0;

    aPDU = createPDU(sequenceNumber, flag, dataBuffer, dataLen);

    if (UTIL_MODE == DEBUG_MODE) {
        outputPDU(aPDU, dataLen + 7);
    }

    if ((sendOutput = sendtoErr(connection->socket, aPDU, dataLen + 7, 0, (struct sockaddr *)&(connection->server), clientAddrLen)) < 0) {
		fprintf(stderr, "Send call output %d\n", sendOutput);
        perror("sendCall");
        exit(1);
    }

	printf("Got to end of send call\n");

    return dataLen;
}

// WINDOW FUNCTIONS
Window *initWindow(uint32_t windowSize, uint32_t bufferSize)
{
  	Window *window = malloc(sizeof(struct Window));
  	window->initialSequenceNumber = 0;
  	window->windowSize = windowSize;
  	window->dataPacketSize = bufferSize;
  	window->bufferSize = windowSize * bufferSize;
  	window->ACKList = malloc(windowSize);
  	window->windowDataBuffer = malloc(windowSize * bufferSize);
  	window->dataLen = window->dataPacketSize * windowSize;
  	resetWindowACK(window);
  	return window;
}

void resetWindowACK(struct Window *window)
{
  	window->windowIndex = 0;
  	memset(window->ACKList, 0, window->windowSize);
}

int isWindowFull(struct Window *window)
{
	int i = 0;
  	// int32_t windowSize = (int32_t) ceil((1.0 * window->dataLen) / window->dataPacketSize);

  	for (i = 0; i < window->windowSize; i++) {
    	if (window->ACKList[i] == 0) {
      		return WINDOW_NOT_FULL;
    	}
  	}

  	return WINDOW_FULL;
}

uint32_t getMaxSequenceNumber(struct Window *window)
{
	int i = 0;
  	uint32_t maxACKIndex = 0;

  	for (i = 0; i < window->windowSize; i++) {
    	if (window->ACKList[i] == 1) {
      		maxACKIndex = i;
    	}
  	}

  	return window->initialSequenceNumber + maxACKIndex;
}

uint32_t getNextSequenceNumber(struct Window *window)
{
	int i  = 0;

	for (i = 0; i < window->windowSize; i++) {
    	if (window->ACKList[i] == 0) {
      		break;
    	}
  	}

  	return window->initialSequenceNumber + i;
}

void freeWindow(struct Window *window)
{
  	if (window != NULL) {
    	if (window->windowDataBuffer != NULL) {
      		free(window->windowDataBuffer);
		}
    	if(window->ACKList != NULL) {
      		free(window->ACKList);
	  	}
	}
    free(window);
}
