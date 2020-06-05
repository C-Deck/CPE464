// Client side - UDP Code				    
// By Hugh Smith	4/1/2017		

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
#include <netdb.h>

#include "gethostbyname.h"
#include"cpe464.h"
#include "util.h"

#define MAXBUF 80
#define xstr(a) str(a)
#define str(a) #a

typedef enum
{
	STATE_START,
	STATE_DONE,
	STATE_FILENAME,
	STATE_FILE_OK,
	STATE_RECV_DATA,
	STATE_WINDOW_FULL,
	STATE_EOF
} STATE;

UDPConnection server;
int MODE = DEBUG_MODE;

void initClient(int argc, char *argv[], struct Client *client);
void runStateMachine(struct Client *client);
STATE windowFull(struct Window *window, int outFile);
STATE recvData(struct Window *window);
STATE recvEOF(struct Window *window, int outFile);
STATE filename(char *fname, int32_t bufferSize, int32_t windowSize);
void sendSREJ(int sequenceNumber);
void sendAck(int sequenceNumber);

int main(int argc, char *argv[])
{
	struct Client client;

	initClient(argc, argv, &client);

	if (MODE == DEBUG_MODE) {
   		printf("windowSize: %d - bufferSize: %d\n", client.windowSize, client.bufferSize);
	}

    sendErr_init(client.errorPercent, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);

    runStateMachine(&client);

	return 0;
}

void initClient(int argc, char *argv[], struct Client *client)
{
	if (argc != 8) {
       fprintf(stderr, "usage: %s <remote_file_name> <local_file_name> <bufferSize> <error_rate> <windowSize> <remote-machine> <remote-port>\n" , argv[0]);
       exit(-2);
    }

    // Save remote file name
    if (strlen(argv[1]) >  MAX_FILE_LENGTH) {
    	fprintf(stderr, "ERROR: Remote filename too long - Maximum 100\n");
    	exit(1);
    }
    client->fromFilename = argv[1];

    // Save local file name
    if (strlen(argv[2]) >  MAX_FILE_LENGTH) {
    	fprintf(stderr, "ERROR: Local filename too long - Maximum 100\n");
    	exit(1);
    }
	client->toFilename = argv[2];

	client->windowSize = atoi(argv[3]);
	client->bufferSize = atoi(argv[4]);
	client->errorPercent = atof(argv[5]);
	client->serverHost = argv[6];
	client->portNumber = atoi(argv[7]);
}

// State Machine
void runStateMachine(struct Client *client)
{
	int outFile = -1;
	int selectCounter = 0;
	int state = STATE_FILENAME;
	struct Window *window = NULL;

	while (state != STATE_DONE) {
		switch (state) {

			case STATE_FILENAME:
			    connectServer(&server, client->serverHost, client->portNumber);
				state = filename(client->fromFilename, client->bufferSize, client->windowSize);
				if (state == STATE_FILENAME) {
					close(server.socket);
				}
				selectCounter++;
				if (selectCounter >= MAX_SELECT_CALLS) {
					fprintf(stderr, "Select timeout occurred\n");
					exit(1);
				}
				break;

			case STATE_FILE_OK:
				selectCounter = 0;
				if ((outFile = open(client->toFilename, O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0 ) {
			    	perror("Open local_file");
			    	exit(1);
			    }
			    window = initWindow(client->windowSize, client->bufferSize);
			    state = STATE_RECV_DATA;
			    break;

			case STATE_RECV_DATA:
				state = recvData(window);
				break;

			case STATE_WINDOW_FULL:
				state = windowFull(window, outFile);
				resetWindowACK(window);
				window->initialSequenceNumber += window->windowSize;
				window->windowByteSize = 0;
				break;

			case STATE_EOF:
				if (selectCounter == 0 && isWindowFull(window)) {
					windowFull(window, outFile);
				}
				selectCounter++;
				if (selectCounter >= MAX_SELECT_CALLS) {
					fprintf(stderr, "Ten failed EOF acks: file is ok but server doesn't know\n");
					state = STATE_DONE;
				}
				else {
					state = recvEOF(window, outFile);
				}
				break;

			case STATE_DONE:
			default:
				break;
		}
	}

	if (window != NULL) {
		freeWindow(window);
		window = NULL;
	}

	close(outFile);
}

STATE filename(char *fname, int32_t bufferSize, int32_t windowSize)
{
	uint8_t dataBuffer[MAX_BUFFER];
	uint8_t flag = 0;
	uint32_t sequenceNumber = 0;
	int32_t filenameLength = strlen(fname) + 1;
	int32_t recvLen = 0;

   ((uint32_t *) dataBuffer)[0] = htonl(bufferSize);
   ((uint32_t *) dataBuffer)[1] = htonl(windowSize);
	memcpy(&(dataBuffer[8]), fname, filenameLength);

   	if (MODE == DEBUG_MODE) {
		printf("Sending file %s\n", (char *) &(dataBuffer[8]));
	}

	if (MODE == DEBUG_MODE) {
   		printf("windowSize: %d - bufferSize: %d\n", ntohl(((uint32_t *) dataBuffer)[0]), ntohl(((uint32_t *) dataBuffer)[1]));
	}

	sendCall(dataBuffer, filenameLength + 8, &server, FILENAME_FLAG, 0);

	if (selectCall(server.socket, 1, 0, TIME_IS_NOT_NULL) == 1) {
		recvLen = recvCall(dataBuffer, MAX_BUFFER, server.socket, &server, &flag, &sequenceNumber);

		if (recvLen == RECV_ERROR) {
			return STATE_FILENAME;
		}

		switch (flag) {
			case FILENAME_GOOD_FLAG:
				return STATE_FILE_OK;
				break;
			case FILENAME_BAD_FLAG:
				printf("File (%s) not found on server\n", fname);
				exit(1);
				break;
			default:
				break;
		}
	}

	return STATE_FILENAME;
}

STATE recvData(struct Window *window)
{
	uint32_t sequenceNumber = 0;
	uint8_t flag = 0;
	int dataLen = 0, i;
	uint8_t dataBuffer[MAX_BUFFER];

	uint32_t windowIndex;
	uint32_t offset;
	uint32_t maxSequenceNumber;
	uint32_t nextSequenceNumber;

	if (selectCall(server.socket, 10, 0, TIME_IS_NOT_NULL) == 0) {
		fprintf(stderr, "Shutting down: No response from server for 10 seconds.\n");
		exit(1);
	}

	dataLen = recvCall(dataBuffer, MAX_BUFFER, server.socket, &server, &flag, &sequenceNumber);

	if (dataLen == RECV_ERROR) {
		if (MODE == DEBUG_MODE) {
			printf("Bad data received - calling again\n");
		}
		return STATE_RECV_DATA;
	}

	switch (flag) {
		case DATA_FLAG:
			maxSequenceNumber = getMaxSequenceNumber(window);
			nextSequenceNumber = getNextSequenceNumber(window);

			if (MODE == DEBUG_MODE) {
				printf("PACKET: sequenceNumber: %u initial: %u max: %u next: %u\n", sequenceNumber, window->initialSequenceNumber, maxSequenceNumber, nextSequenceNumber);
				printWindow(window);
			}

			if (sequenceNumber < nextSequenceNumber) {
				if (sequenceNumber < window->initialSequenceNumber) {
					sendAck(window->initialSequenceNumber - 1);
				}
				else {
					if (isWindowFull(window)) {
						printf("Window is full - RR max seq\n");
						sendAck(maxSequenceNumber);
					}
					else {
						sendSREJ(nextSequenceNumber);
					}
				}
				break;
			}

			// See if the sequenceNumber falls within this windows range
			if ((sequenceNumber >= window->initialSequenceNumber) && (sequenceNumber < (window->initialSequenceNumber + window->maxWindowIndex))) {
				windowIndex = (sequenceNumber - 1) % window->windowSize;
				// Save data if it isn't already in there
				if (window->ACKList[windowIndex] == 0) {

					offset = windowIndex * window->dataPacketSize;
					memcpy(&window->windowDataBuffer[offset], dataBuffer, dataLen);

					// Mark the window as received
					window->ACKList[windowIndex] = 1;

					maxSequenceNumber = getMaxSequenceNumber(window);
					nextSequenceNumber = getNextSequenceNumber(window);



					if (sequenceNumber == maxSequenceNumber) {
						window->windowByteSize = offset + dataLen;
					}
				}

				if (MODE == DEBUG_MODE) {
					printf("PACKET WRITEN: sequenceNumber: %u initial: %u max: %u next: %u\n", sequenceNumber, window->initialSequenceNumber, maxSequenceNumber, nextSequenceNumber);
					printWindow(window);
				}
			}
			


			if (sequenceNumber > nextSequenceNumber) {
				// Check the numbers bellow it
				for (i = nextSequenceNumber - 1; i < sequenceNumber; i++) {
					windowIndex = i % window->windowSize;
					if (window->ACKList[windowIndex-1] == 0) {
						sendSREJ(window->initialSequenceNumber + windowIndex - 1);
						break;
					}
				}
			}
			else {
				printf("RR: Doing next seq - 1\n");
				sendAck(nextSequenceNumber - 1);
			}

			if (isWindowFull(window)) {
				if (MODE == DEBUG_MODE) {
					printf("Window Full\n");
				}
				return STATE_WINDOW_FULL;
			}
			break;
		case DATA_EOF_FLAG:
			if (MODE == DEBUG_MODE) {
				printf("GOT EOF: sequenceNumber: %u\n", sequenceNumber);
			}
			return STATE_EOF;
			break;
		default:
			break;

	}

	return STATE_RECV_DATA;
}

STATE windowFull(struct Window *window, int outFile)
{
	if (MODE == DEBUG_MODE) {
		uint32_t window_count = getNextSequenceNumber(window) - window->initialSequenceNumber;
		printf("Writing %d windows %u bytes\n", window_count, window->windowByteSize);
	}

	write(outFile, window->windowDataBuffer, window->windowByteSize);
	return STATE_RECV_DATA;
}

STATE recvEOF(struct Window *window, int outFile)
{
	uint8_t flag = 0;
	uint8_t dataBuffer[MAX_BUFFER];
	uint32_t sequenceNumber = getNextSequenceNumber(window);

	sendCall(NULL, 0, &server, ACK_EOF_FLAG, sequenceNumber);
	// Drain the packet queue
	while (selectCall(server.socket, 1, 0, TIME_IS_NOT_NULL) == 1) {
		recvCall(dataBuffer, MAX_BUFFER, server.socket, &server, &flag, &sequenceNumber);
		return STATE_EOF;
	}

	printf("File Transfer Complete\n");
	return STATE_DONE;
}

void sendAck(int sequenceNumber)
{
	if (MODE == DEBUG_MODE) {
		printf("Sending RR: %u\n", sequenceNumber);
	}

	sendCall(NULL, 0, &server, RR_FLAG, sequenceNumber);
}

void sendSREJ(int sequenceNumber)
{
	if (MODE == DEBUG_MODE) {
		printf("Sending SREJ: %u\n", sequenceNumber);
	}

	sendCall(NULL, 0, &server, SREJ_FLAG, sequenceNumber);
}
