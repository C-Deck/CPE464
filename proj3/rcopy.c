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

void initClient(int argc, char *argv[], struct Client *client);
void runStateMachine(struct Client *client);
STATE windowFull(struct Window *window, int output_fd);
STATE recvData(struct Window *window);
STATE recvEOF(struct Window *window, int output_fd);
STATE filename(char *fname, int32_t buf_size, int32_t windowSize);
void sendSREJ(int sequenceNumber);
void sendAck(int sequenceNumber);

int main(int argc, char *argv[])
{
	struct Client client;

	initClient(argc, argv, &client);

    sendErr_init(client.errorPercent, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);

    runStateMachine(&client);

	return 0;
}

void initClient(int argc, char *argv[], struct Client *client)
{
	if (argc != 8) {
       fprintf(stderr, "usage: %s <remote_file_name> <local_file_name> <buf_size> <error_rate> <windowSize> <remote-machine> <remote-port>\n" , argv[0]);
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
	int output_fd = -1;
	int select_count = 0;
	int state = STATE_FILENAME;
	struct Window *window = NULL;

	while (state != STATE_DONE) {
		switch (state) {

			case STATE_FILENAME:
				if (connectServer(&server, client->serverHost, client->portNumber)) {
					perror("ERROR: unable to connect to host");
					exit(1);
				}
				state = filename(client->fromFilename, client->bufferSize, client->windowSize);
				if (state == STATE_FILENAME) {
					close(server.socket);
				}
				select_count++;
				if (select_count >= MAX_SELECT_CALLS) {
					fprintf(stderr, "Select timeout occurred\n");
					exit(1);
				}
				break;

			case STATE_FILE_OK:
				select_count = 0;
				if ((output_fd = open(client->toFilename, O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0 ) {
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
				state = windowFull(window, output_fd);
				resetWindowACK(window);
				window->initialSequenceNumber += window->windowSize;
				window->bufferSize = 0;
				break;

			case STATE_EOF:
				if (select_count == 0) {
					windowFull(window, output_fd);
				}
				select_count++;
				if (select_count >= MAX_SELECT_CALLS) {
					fprintf(stderr, "Ten failed EOF acks: file is ok but server doesn't know\n");
					state = STATE_DONE;
				}
				else {
					state = recvEOF(window, output_fd);
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

	close(output_fd);
}

STATE filename(char *fname, int32_t buf_size, int32_t windowSize)
{
	uint8_t buf[MAX_BUFFER];
	uint8_t flag = 0;
	uint32_t sequenceNumber = 0;
	int32_t fname_len = strlen(fname) + 1;
	int32_t recvLen = 0;

	buf_size = htonl(buf_size);
	memcpy(buf, &buf_size, 4);
	windowSize = htonl(windowSize);
	memcpy(&buf[4], &windowSize, 4);
	memcpy(&buf[8], fname, fname_len);

	sendCall(buf, fname_len + 8, &server, FILENAME_FLAG, 0);

	if (selectCall(server.socket, 1, 0, TIME_IS_NOT_NULL) == 1) {
		recvLen = recvCall(buf, MAX_BUFFER, server.socket, &server, &flag, &sequenceNumber);

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
	int32_t dataLen = 0;
	uint8_t dataBuffer[MAX_BUFFER];

	uint32_t windowIndex;
	uint32_t offset;
	uint32_t maxSequenceNumber;

	static uint32_t expected_seq_number = 0;

	if (selectCall(server.socket, 10, 0, TIME_IS_NOT_NULL) == 0) {
		fprintf(stderr, "Shutting down: No response from server for 10 seconds.\n");
		exit(1);
	}

	dataLen = recvCall(dataBuffer, MAX_BUFFER, server.socket, &server, &flag, &sequenceNumber);

	if (dataLen == RECV_ERROR) {
		return STATE_RECV_DATA;
	}

	switch (flag) {
		case DATA_FLAG:
			maxSequenceNumber = getMaxSequenceNumber(window);
			maxSequenceNumber = getNextSequenceNumber(window);

			// printf("PACKET: sequenceNumber: %u max: %u next: %u exp: %u\n", sequenceNumber, maxSequenceNumber, maxSequenceNumber, expected_seq_number);

			if (sequenceNumber < maxSequenceNumber) {
				if (sequenceNumber < window->initialSequenceNumber) {
					sendAck(window->initialSequenceNumber-1);
				}
				else {
					if (isWindowFull(window)) {
						sendAck(maxSequenceNumber);
					}
					else {
						sendSREJ(maxSequenceNumber);
					}
				}
				break;
			}

			// See if the sequenceNumber falls within this windows range
			if ((sequenceNumber >= window->initialSequenceNumber) && (sequenceNumber < (window->initialSequenceNumber + window->windowSize))) {
				windowIndex = (sequenceNumber - 1) % window->windowSize;
				// Save data if it isn't already in there
				if (window->ACKList[windowIndex] == 0) {

					offset = windowIndex * window->dataPacketSize;
					memcpy(&window->windowDataBuffer[offset], dataBuffer, dataLen);

					// Mark the window as received
					window->ACKList[windowIndex] = 1;

					maxSequenceNumber = getMaxSequenceNumber(window);
					maxSequenceNumber = getNextSequenceNumber(window);

					if (sequenceNumber == maxSequenceNumber) {
						window->bufferSize = offset + dataLen;
					}
				}
			}


			if (sequenceNumber > maxSequenceNumber) {
				// the previous sequence number
				windowIndex = ((sequenceNumber-1) % window->windowSize);
				if (window->ACKList[windowIndex-1] == 0) {
					sendSREJ(window->initialSequenceNumber + windowIndex - 1);
				}
			}
			else {
				sendAck(maxSequenceNumber - 1);
			}

			expected_seq_number = maxSequenceNumber;

			if (isWindowFull(window)) {
				// printf("Window Full\n");
				return STATE_WINDOW_FULL;
			}

			break;

		case DATA_EOF_FLAG:
			// printf("GOT EOF: sequenceNumber: %u\n", sequenceNumber);// !!!
			return STATE_EOF;
			break;

		default:
			break;

	}

	return STATE_RECV_DATA;
}

STATE windowFull(struct Window *window, int output_fd)
{
	// uint32_t window_count = nextSequenceNumber(window) - window->initialSequenceNumber;
	// printf("Writing %d windows %u bytes\n", window_count, window->windowDataBufferSize); // !!!
	write(output_fd, window->windowDataBuffer, window->bufferSize);
	return STATE_RECV_DATA;
}

STATE recvEOF(struct Window *window, int output_fd)
{
	uint8_t flag = 0;
	uint8_t dataBuffer[MAX_BUFFER];
	uint32_t sequenceNumber = getNextSequenceNumber(window);

	sendAck(sequenceNumber);
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
	// printf("SEND RR: %u\n", sequenceNumber);
	sendCall(NULL, 0, &server, RR_FLAG, sequenceNumber);
}

void sendSREJ(int sequenceNumber)
{
	// printf("SEND SREJ: %u\n", sequenceNumber);
	sendCall(NULL, 0, &server, SREJ_FLAG, sequenceNumber);
}