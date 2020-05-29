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

#include "gethostbyname.h"
#include "util.h"

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

void processClient(int socketNum);
int checkArgs(int argc, char *argv[]);
void receiveClients();
void processClient(uint8_t *dataBuffer, int32_t dataLen, struct UDPConnection *client, int nest_level);
STATE closeWindow(struct UDPConnection *client, struct Window *window, int nest_level);
STATE getFilename(struct UDPConnection *client, uint8_t *dataBuffer, int32_t dataLen, int32_t *data_file, struct Window **window);
STATE nextWindow(struct Window *window, int data_file);
STATE nextDataPacket(struct UDPConnection *client, struct Window *window);
STATE readAck(struct UDPConnection *client, struct Window *window, int nest_level);
void sendDataPacket(struct UDPConnection *client, struct Window *window, uint32_t sequenceNumber);
STATE closeWindow(struct UDPConnection *client, struct Window *window, int nest_level);
STATE sendEOF(struct UDPConnection *client, Window *window);

//TODO circular queue

int main (int argc, char *argv[])
{
	double errorRate = 0;
	int portNumber = 0;

	portNumber = checkArgs(argc, argv);
	errorRate = atof(argv[3]);
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
void processClient(uint8_t *dataBuffer, int32_t dataLen, struct UDPConnection *client, int nest_level)
{
	STATE state = STATE_START;

	int32_t data_file = 0;

	Window *window = NULL;

	if (nest_level < MAX_SELECT_CALLS) {
		while (state != STATE_DONE) {
			switch (state) {

				case STATE_START:
					state = STATE_FILENAME;
					break;

				case STATE_FILENAME:
					state = getFilename(client, dataBuffer, dataLen, &data_file, &window);
					break;

				case STATE_WINDOW_NEXT:
					state = nextWindow(window, data_file);
					break;

				case STATE_WINDOW_CLOSE:
					state = closeWindow(client, window, nest_level);
					break;

				case STATE_SEND_DATA:
					state = nextDataPacket(client, window);
					break;

				case STATE_SEND_EOF:
					state = sendEOF(client, window);
					break;

				case STATE_READ_ACKS:
					state = readAck(client, window, nest_level);
					break;

				case STATE_DONE:
				default:
					state = STATE_DONE;
					break;
			}
		}
	}

	if (window != NULL) {
		destroyWindow(window);
		window = NULL;
	}
}

STATE getFilename(struct UDPConnection *client, uint8_t *dataBuffer, int32_t dataLen, int32_t *data_file, struct Window **window)
{
	char fname[MAX_FILE_LENGTH];
	uint32_t bufferSize;
	uint32_t windowSize;

	windowSize = ntohl(*((uint16_t *) dataBuffer));
	bufferSize = ntohl(*((uint16_t *) &(dataBuffer[4])));

	if ((client->socket = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		perror("filename socket call");
		exit(1);
	}

	if ((dataLen - 8) > MAX_FILE_LENGTH) {
		sendCall(NULL, 0, client, FILENAME_BAD_FLAG, 0);
		return STATE_DONE;
	}

	memcpy(fname, &dataBuffer[8], dataLen - 8);

	if (((*data_file) = open(fname, O_RDONLY)) < 0) {
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

STATE nextWindow(struct Window *window, int data_file)
{
	int32_t readLen = 0;
	// printf("Loading window base: %u length: %u size: %u\n", window->base_seq_num, window->dataPacketSize, window->window_size); // !!!
	
	readLen = read(data_file, window->windowDataBuffer, window->windowDataBufferSize);

	resetWindowACK(window);
	window->windowIndex = 0;

	switch (readLen) {
		case -1:
			perror("sendDataPacket: read");
			exit(1);
			break;

		case 0:
			return STATE_SEND_EOF;
			break;

		default:
			window->dataLen = readLen;
			break;
	}

	return STATE_SEND_DATA;
}

STATE nextDataPacket(struct UDPConnection *client, struct Window *window)
{
	STATE returnState = STATE_READ_ACKS;

	uint32_t window_buffer_offset = window->windowIndex * window->dataPacketSize;
	uint32_t packetLen = window->dataPacketSize;

	int32_t data_left = window->dataLen - window_buffer_offset;

	if (data_left <= window->dataPacketSize) {
		packetLen = data_left;
		returnState = STATE_WINDOW_CLOSE;
	}

	// printf("SEND %u\n", window->base_seq_num + window->windowIndex);
	sendCall(window->windowDataBuffer + window_buffer_offset, packetLen, client, DATA_FLAG, window->base_seq_num + window->windowIndex);

	window->windowIndex++;

	return returnState;
}

STATE readAck(struct UDPConnection *client, struct Window *window, int nest_level)
{
	uint32_t recvLen = 0;
	uint8_t dataBuffer[MAX_BUFFER];
	uint8_t flag = 0;
	uint32_t sequenceNumber = 0;
	uint32_t windowIndex;
	int i = 0;

	while(selectCall(client->socket, 0, 0, TIME_IS_NOT_NULL)) {
		recvLen = recvCall(dataBuffer, MAX_BUFFER, client->socket_num, client, &flag, &sequenceNumber);

		if (recvLen == RECV_ERROR) {
			return STATE_SEND_DATA;
		}

		if ((sequenceNumber >= window->base_seq_num) && (sequenceNumber < (window->base_seq_num + window->window_size))) {
			windowIndex = (sequenceNumber-1) % window->window_size;
			switch (flag) {
				case FILENAME_FLAG:
					processClient(dataBuffer, recvLen, client, nest_level + 1);
					return STATE_DONE;
					break;

				case RR_FLAG:
					// printf("RR %u\n", sequenceNumber);
					for (i=0; i < windowIndex; i++) {
						window->ACKList[i] = 1;
					}
					break;

				case SREJ_FLAG:
					// printf("SREJ %u\n", sequenceNumber);
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
	// printf("RESEND %u\n", sequenceNumber); // !!!

	uint32_t windowIndex = (sequenceNumber-1) % window->window_size;

	uint32_t window_buffer_offset = windowIndex * window->dataPacketSize;

	uint32_t packet_len = window->dataPacketSize;

	int32_t data_left = window->windowDataBufferSize - window_buffer_offset;

	if (data_left < window->dataPacketSize) {
		packet_len = data_left;
	}

	sendCall(window->windowDataBuffer + window_buffer_offset, packet_len, client, DATA_FLAG, sequenceNumber);
}

STATE closeWindow(struct UDPConnection *client, struct Window *window, int nest_level)
{
	uint8_t dataBuffer[MAX_BUFFER];
	uint8_t flag = 0;
	uint32_t sequenceNumber = 0, windowIndex = 0, recvLen = 0;
	int sendCount = 0, i;

	while (1) {

		if (isWindowFull(window)) {
			// printf("!!! Window Full next_base: %u\n", nextSequenceNumber(window));
			window->base_seq_num = nextSequenceNumber(window);
			return STATE_WINDOW_NEXT;
		}

		if(selectCall(client->socket, 1, 0, TIME_IS_NOT_NULL)) {
			recvLen = recvCall(dataBuffer, MAX_BUFFER, client->socket, client, &flag, &sequenceNumber);

			if (recvLen == RECV_ERROR) {
				continue;
			}

			if ((sequenceNumber >= window->base_seq_num) && (sequenceNumber < (window->base_seq_num + window->window_size))) {

				windowIndex = (sequenceNumber-1) % window->window_size;

				switch (flag) {
					case RR_FLAG:
						sendCount = 0;
						for (i= 0; i <= windowIndex; i++) {
							window->ACKList[i] = 1;
						}
						// printf("ACK %u\n", sequenceNumber);
						break;

					case SREJ_FLAG:
						sendCount = 0;
						// printf("SREJ %u\n", sequenceNumber);
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
			// printf("Timeout!\n"); // !!!
			sendDataPacket(client, window, window->base_seq_num);
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
		// printf("Send EOF: base: %u ACKList: %u\n", window->base_seq_num, window->ACKList[0]);
		//TODO Come back - why base_seq_num
		sendCall(NULL, 0, client, DATA_EOF_FLAG, window->base_seq_num);

		selectCounter++;

		while(selectCall(client->socket, 1, 0, TIME_IS_NOT_NULL)) {
			recvLen = recvCall(dataBuffer, MAX_BUFFER, client->socket, client, &flag, &sequenceNumber);

			if (recvLen == RECV_ERROR) {
				continue;
			}

			switch (flag) {
				//TODO Come back to this
				case SREJ_FLAG:
				case ACK_EOF_FLAG:
					if (sequenceNumber == window->base_seq_num) {
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

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}


