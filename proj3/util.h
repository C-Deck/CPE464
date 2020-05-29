#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_BUFFER 2048
#define MAX_FILE_LENGTH 100

#define RECV_ERROR -1

#define TIME_IS_NULL 1
#define TIME_IS_NOT_NULL 2

#define NORMAL_MODE 1
#define DEBUG_MODE 2

#define WINDOW_NOT_FULL 0
#define WINDOW_FULL 1

#define MAX_SELECT_CALLS 10

// Flags for the packets
#define SETUP_RCOPY_FLAG 0x1		// Not used
#define SETUP_SERVER_FLAG 0x2		// Not used
#define DATA_FLAG 0x3
#define RR_FLAG 0x5
#define SREJ_FLAG 0x6
#define FILENAME_FLAG 0x7
#define FILENAME_GOOD_FLAG 0x8
#define FILENAME_BAD_FLAG 0xA
#define DATA_EOF_FLAG 0xB
#define ACK_EOF_FLAG 0xC
#define END_RCOPY_FLAG 0xD
#define END_SERVER_FLAG 0xE

typedef struct
{
  int32_t socket;
  struct sockaddr_in6 server;
} UDPConnection;

typedef struct
{
	uint8_t *ACKList;
	uint8_t *windowDataBuffer;
	uint32_t windowSize;
	uint32_t dataPacketSize;
	uint32_t bufferSize;
	uint32_t dataLen;
	uint32_t initialSequenceNumber;
	uint32_t windowIndex;
} Window;

typedef struct
{
	char *fromFilename;
	char *toFilename;
	uint32_t windowSize;
	uint32_t bufferSize;
	double errorPercent;
	char *serverHost;
	uint16_t portNumber;
} Client;

void outputPDU(uint8_t * aPDU, int pduLength);
uint8_t * createPDU(uint32_t sequenceNumber, uint8_t flag, uint8_t *payload, int dataLen);
int startServer(int portNumber);
int connectServer(struct UDPConnection *udp, char * hostName, int portNumber);
int32_t selectCall(int32_t socketNumber, int32_t seconds, int32_t microseconds, int32_t timeIsNotNull);
int32_t recvCall(uint8_t *dataBuffer, uint32_t len, int32_t socket, UDPConnection *connection, uint8_t *flag, uint32_t *sequenceNumber);
int32_t baseRecvCall(uint8_t *dataBuffer, uint32_t len, int32_t socket, UDPConnection *connection);
int32_t sendCall(uint8_t *buf, uint32_t dataLen, UDPConnection *connection, uint8_t flag, uint32_t sequenceNumber);
Window *initWindow(uint32_t windowSize, uint32_t bufferSize);
uint32_t getMaxSequenceNumber(struct Window *window);
uint32_t getNextSequenceNumber(struct Window *window);
int isWindowFull(struct Window *window);
void resetWindowACK(struct Window *window);
void freeWindow(struct Window *window);

#endif