/* Server code - UDP Code		*/
/* By Clint Decker	5/28/2020	*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "gethostbyname.h"
#include "util.h"
#include "cpe464.h"

typedef enum
{
	STATE_START,
	STATE_DONE,
	STATE_FILENAME,
	STATE_WINDOW_NEXT,
	STATE_WINDOW_CLOSE,
	STATE_SEND_DATA,
	STATE_SEND_EOF,
	STATE_READ_ACKS,
	STATE_READ_EOF_ACK
} STATE;

int MODE = DEBUG_MODE;

int checkArgs(int argc, char *argv[]);
void receiveClients(int portNumber);
void processClient(uint8_t *dataBuffer, int32_t dataLen, struct UDPConnection *client, int selectFailureCount);
STATE closeWindow(struct UDPConnection *client, struct Window *window, int selectFailureCount);
STATE getFilename(struct UDPConnection *client, uint8_t *dataBuffer, int32_t dataLen, int32_t *dataFile, struct Window **window);
STATE nextWindow(struct Window *window, int dataFile);
STATE nextDataPacket(struct UDPConnection *client, struct Window *window);
STATE readAck(struct UDPConnection *client, struct Window *window, int selectFailureCount);
void sendDataPacket(struct UDPConnection *client, struct Window *window, uint32_t sequenceNumber);
STATE closeWindow(struct UDPConnection *client, struct Window *window, int selectFailureCount);
STATE sendEOF(struct UDPConnection *client, Window *window);

//TODO circular queue

int main (int argc, char *argv[])
{
	double errorRate = 0;
	int portNumber = 0;

	portNumber = checkArgs(argc, argv);
	errorRate = atof(argv[1]);
	sendtoErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);

	receiveClients(portNumber);

	return 0;
}

// Handle the forking here
void receiveClients(int portNumber)
{
	// Variables for forking
	pid_t pid = 0;
	int status;

	uint8_t dataBuffer[MAX_BUFFER];
	UDPConnection clientConnection;
	int socketNum = 0;
	uint32_t dataLen = 0;
	
	socketNum = startServer(portNumber);

	while (1)
	{
		if (selectCall(socketNum, 1, 0, TIME_IS_NOT_NULL) == 1) {
			// Socket has data - Recv initial packet
			dataLen = baseRecvCall(dataBuffer, MAX_BUFFER, socketNum, &clientConnection);

			if (dataLen != RECV_ERROR) {
				if ((pid = fork()) < 0) {
					perror("mainServerRunLoop:fork");
					exit(1);
				}

				// We are child
				if (pid == 0) {
					processClient(dataBuffer, dataLen, &clientConnection, 0);
					exit(0);
				}

				while (waitpid(-1, &status, WNOHANG) > 0) {
				}
			}
		}
	}

	close(socketNum);
}

// State Machine
void processClient(uint8_t *dataBuffer, int32_t dataLen, struct UDPConnection *client, int selectFailureCount)
{
	STATE state = STATE_START;

	int32_t dataFile = 0;

	Window *window = NULL;

	if (selectFailureCount < MAX_SELECT_CALLS) {
		while (state != STATE_DONE) {
			switch (state) {

				case STATE_START:
					state = STATE_FILENAME;
					break;

				case STATE_FILENAME:
					state = getFilename(client, dataBuffer, dataLen, &dataFile, &window);
					break;

				case STATE_WINDOW_NEXT:
					state = nextWindow(window, dataFile);
					break;

				case STATE_WINDOW_CLOSE:
					state = closeWindow(client, window, selectFailureCount);
					break;

				case STATE_SEND_DATA:
					state = nextDataPacket(client, window);
					break;

				case STATE_SEND_EOF:
					state = sendEOF(client, window);
					break;

				case STATE_READ_ACKS:
					state = readAck(client, window, selectFailureCount);
					break;

				case STATE_DONE:
				default:
					state = STATE_DONE;
					break;
			}
		}
	}

	if (window != NULL) {
		freeWindow(window);
		window = NULL;
	}
}

STATE getFilename(struct UDPConnection *client, uint8_t *dataBuffer, int32_t dataLen, int32_t *dataFile, struct Window **window)
{
	char fname[MAX_FILE_LENGTH];
	uint32_t bufferSize;
	uint32_t windowSize;

	bufferSize = ntohl(((uint32_t *) dataBuffer)[0]);
	windowSize = ntohl(((uint32_t *) dataBuffer)[1]);

	if (MODE == DEBUG_MODE) {
   		printf("windowSize: %d - bufferSize: %d\n", windowSize, bufferSize);
	}

	if ((client->socket = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		perror("filename socket call");
		exit(1);
	}

	if ((dataLen - 8) > MAX_FILE_LENGTH) {
		sendCall(NULL, 0, client, FILENAME_BAD_FLAG, 0);
		return STATE_DONE;
	}

	memcpy(fname, &(dataBuffer[8]), dataLen - 8);
    fname[dataLen - 8 - 1] = '\0';

	if (MODE == DEBUG_MODE) {
   		printf("Filename: %s - Length: %d - DataLen: %d\n", fname, (int) strlen(fname), dataLen);
	}

	if ((*dataFile = open(fname, O_RDONLY)) < 0) {
		sendCall(NULL, 0, client, FILENAME_BAD_FLAG, 0);
		free(*window);
		*window = NULL;
		return STATE_DONE;
	}

	sendCall(NULL, 0, client, FILENAME_GOOD_FLAG, 0);

	if (window != NULL) {
		*window = initWindow(windowSize, bufferSize);
	}

	return STATE_WINDOW_NEXT;
}

STATE nextWindow(struct Window *window, int dataFile)
{
	int readLen = 0;

	if (MODE == DEBUG_MODE) {
		printf("New Window - Initial: %u - Length: %u - Size: %u\n", window->initialSequenceNumber, window->dataPacketSize, window->windowSize);
	}

	resetWindowACK(window);
	
	if ((readLen = read(dataFile, window->windowDataBuffer, window->windowByteSize)) == -1) {
		perror("Read error ");
		exit(1);
	} 

	else if (readLen == 0) {
		return STATE_SEND_EOF;
	} 

	else {
		if (MODE == DEBUG_MODE) {
			printf("Read %u bytes\n", readLen);
		}
		window->dataLen = readLen;
		window->maxWindowIndex = (readLen + window->dataPacketSize - 1) / window->dataPacketSize;
		return STATE_SEND_DATA;
	}
}

STATE nextDataPacket(struct UDPConnection *client, struct Window *window)
{
	STATE returnState = STATE_READ_ACKS;

	uint32_t windowBufferOffset = window->windowIndex * window->dataPacketSize;
	uint32_t packetLen = window->dataPacketSize;

	int32_t data_left = window->dataLen - windowBufferOffset;

	if (data_left <= window->dataPacketSize) {
		packetLen = data_left;
		returnState = STATE_WINDOW_CLOSE;
	}

	if (MODE == DEBUG_MODE) {
		printf("SEND %u\n", window->initialSequenceNumber + window->windowIndex);
	}

	sendCall(window->windowDataBuffer + windowBufferOffset, packetLen, client, DATA_FLAG, window->initialSequenceNumber + window->windowIndex);

	window->windowIndex++;

	return returnState;
}

STATE readAck(struct UDPConnection *client, struct Window *window, int selectFailureCount)
{
	uint32_t recvLen = 0;
	uint8_t dataBuffer[MAX_BUFFER];
	uint8_t flag = 0;
	uint32_t sequenceNumber = 0;
	uint32_t windowIndex;
	int i = 0;

	while(selectCall(client->socket, 0, 0, TIME_IS_NOT_NULL)) {
		recvLen = recvCall(dataBuffer, MAX_BUFFER, client->socket, client, &flag, &sequenceNumber);

		if (recvLen == RECV_ERROR) {
			return STATE_SEND_DATA;
		}

		if ((sequenceNumber >= window->initialSequenceNumber) && (sequenceNumber < (window->initialSequenceNumber + window->windowSize))) {
			windowIndex = (sequenceNumber-1) % window->windowSize;
			switch (flag) {
				case FILENAME_FLAG:
					processClient(dataBuffer, recvLen, client, selectFailureCount + 1);
					return STATE_DONE;
					break;

				case RR_FLAG:
					if (MODE == DEBUG_MODE) {
						printf("RR Received: %u\n", sequenceNumber);
					}
					for (i=0; i < windowIndex; i++) {
						window->ACKList[i] = 1;
					}
					break;

				case SREJ_FLAG:
					if (MODE == DEBUG_MODE) {
						printf("SREJ Recevied %u\n", sequenceNumber);
					}
					sendDataPacket(client, window, sequenceNumber);
					break;

				default:
					fprintf(stderr, "ERROR: Bad flag (%u) in readACK\n", flag);
					break;
			}
		}
	}

	return STATE_SEND_DATA;
}

void sendDataPacket(struct UDPConnection *client, struct Window *window, uint32_t sequenceNumber)
{
	if (MODE == DEBUG_MODE) {
		printf("RESEND %u\n", sequenceNumber);
	}

	uint32_t windowIndex = (sequenceNumber - 1) % window->windowSize;

	uint32_t windowBufferOffset = windowIndex * window->dataPacketSize;

	uint32_t packet_len = window->dataPacketSize;

	int32_t data_left = window->windowByteSize - windowBufferOffset;

	if (data_left < window->dataPacketSize) {
		packet_len = data_left;
	}

	sendCall(window->windowDataBuffer + windowBufferOffset, packet_len, client, DATA_FLAG, sequenceNumber);
}

STATE closeWindow(struct UDPConnection *client, struct Window *window, int selectFailureCount)
{
	uint8_t dataBuffer[MAX_BUFFER];
	uint8_t flag = 0;
	uint32_t sequenceNumber = 0, windowIndex = 0, recvLen = 0, nextSequenceNumber = 0;
	int sendCount = 0;

	if (MODE == DEBUG_MODE) {
		printf("Closing window\n");
	}

	while (1) {

		if (isWindowFull(window)) {
			nextSequenceNumber = getNextSequenceNumber(window);
			if (MODE == DEBUG_MODE) {
				printf("Window Full next: %u\n", nextSequenceNumber);
			}
			window->initialSequenceNumber = nextSequenceNumber;
			return STATE_WINDOW_NEXT;
		}

		if (selectCall(client->socket, 1, 0, TIME_IS_NOT_NULL)) {
			recvLen = recvCall(dataBuffer, MAX_BUFFER, client->socket, client, &flag, &sequenceNumber);

			if (recvLen == RECV_ERROR) {
				continue;
			}

			if ((sequenceNumber >= window->initialSequenceNumber) && (sequenceNumber < (window->initialSequenceNumber + window->windowSize))) {

				windowIndex = (sequenceNumber - 1) % window->windowSize;

				switch (flag) {
					case RR_FLAG:
						sendCount = 0;
						window->ACKList[windowIndex] = 1;
						if (MODE == DEBUG_MODE) {
							printf("Received RR %u\n", sequenceNumber);
						}
						break;

					case SREJ_FLAG:
						sendCount = 0;
						if (MODE == DEBUG_MODE) {
							printf("Received SREJ %u\n", sequenceNumber);
						}
						sendDataPacket(client, window, sequenceNumber);
						break;

					default:
						fprintf(stderr, "ERROR: non-ACK flag in wait_on_ack\n");
						break;
				}
			}
		}
		else {
			sendCount++;
			if (sendCount > MAX_SELECT_CALLS) {
				fprintf(stderr, "ERROR: No Response from client in 10 seconds");
				exit(1);
			}

			sendDataPacket(client, window, window->initialSequenceNumber);
		}
	}


	return STATE_DONE;
}

STATE sendEOF(struct UDPConnection *client, Window *window)
{
	uint32_t recvLen = 0, sequenceNumber = 0;
	uint8_t dataBuffer[MAX_BUFFER];
	uint8_t flag = 0;
	int selectCounter = 0;

	while (1) {
		if (MODE == DEBUG_MODE) {
			printf("Send EOF: base: %u ACKList: %u\n", window->initialSequenceNumber, window->ACKList[0]);
		}

		sendCall(NULL, 0, client, DATA_EOF_FLAG, window->initialSequenceNumber);

		selectCounter++;

		while(selectCall(client->socket, 1, 0, TIME_IS_NOT_NULL)) {
			recvLen = recvCall(dataBuffer, MAX_BUFFER, client->socket, client, &flag, &sequenceNumber);

			if (recvLen == RECV_ERROR) {
				continue;
			}

			switch (flag) {
				case SREJ_FLAG:
					if (MODE == DEBUG_MODE) {
						printf("Received SREJ %u\n", sequenceNumber);
					}
					sendDataPacket(client, window, sequenceNumber);
				case ACK_EOF_FLAG:
					if (sequenceNumber == window->initialSequenceNumber) {
						return STATE_DONE;
					}
					break;
				default:

					fprintf(stderr, "ERROR: Bad flag (%u) in EOF_ACK\n", flag);
					break;
			}
		}

		if (selectCounter >= MAX_SELECT_CALLS) {
			fprintf(stderr, "ERROR: 10 failed EOF tries.\n");
			exit(1);
		}
	}

	return STATE_DONE;
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 3)
	{
		fprintf(stderr, "Usage %s error-rate [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 3)
	{
		portNumber = atoi(argv[2]);
	}
	
	return portNumber;
}
