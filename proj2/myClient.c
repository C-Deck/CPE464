/******************************************************************************
* myClient.c
*
*****************************************************************************/

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
#include <ctype.h>

#include "networks.h"
#include "error.h"
#include "safeSystemUtil.h"
#include "pollLib.h"

#define DEBUG_FLAG 1

struct ClientInfo {
	int socket;
	char handle[MAX_HANDLE_LENGTH];
	uint8_t handleLength;
};

void sendToServer(int socketNum, struct ClientInfo *client);
void checkArgs(int argc, char * argv[]);
int getFromStdin(char * sendBuf, char * prompt);
int initClient(struct ClientInfo *client, int socketNum, char *handle);
int initialPacketCheck(struct ClientInfo *client, int socketNum);
int getInitPacketResponse(struct ClientInfo *client, int socketNum);
int parseInput(char *inputBuf, uint16_t *sendLen, uint8_t *packet, struct ClientInfo *client);
void buildBroadcast(char *inputBuf, uint16_t *sendLen, uint8_t *packet, struct ClientInfo *client);
void buildMessage(char *inputBuf, uint16_t *sendLen, uint8_t *packet, struct ClientInfo *client);
void addHandles(char *inputBuf, uint16_t *sendLen, uint8_t *packet, uint8_t numHandles);
void setSender(uint8_t *packet, struct ClientInfo *client);
void addMessage(char *inputBuf, uint16_t *sendLen, uint8_t *packet);
void receiveHandleNumbers(int socketNum);
void allHandlesReceived(int socketNum);
void receiveHandles(int socketNum, uint32_t numberHandles);
void recvServer(int socketNum);
void extractHandle(char *packet, char *handleBuff, uint8_t *handleLen);
void receivedBadDest(char *packet);
void receivedMessage(char *packet, uint16_t packetLength);
void receivedBroadcast(char *packet, uint16_t packetLength);
void parsePacket(char *packet, uint16_t packetLength, uint8_t flag);

// TODO Break messages into multiple packets
int main(int argc, char * argv[])
{
	int socketNum = 0; //socket descriptor
	struct ClientInfo client;
	
	checkArgs(argc, argv);

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[1], argv[2], DEBUG_FLAG);

	if (initClient(&client, socketNum, argv[2]) == 0) {
		if (initialPacketCheck(&client, socketNum) == 0) {
			addToPollSet(socketNum);
			sendToServer(socketNum, &client);
		}
	}
	
	safeClose(socketNum);
	
	return 0;
}

int initClient(struct ClientInfo *client, int socketNum, char *handle)
{
	uint8_t handleLen = 0;

	client->socket = socketNum;
	memset(client->handle, '\0', MAX_HANDLE_LENGTH);

	// Check handle length
    if ((handleLen = strlen(handle)) > MAX_HANDLE_LENGTH) {
    	handleTooLong(handle);
    	return -1;
    }

	client->handleLength = handleLen;

	strncpy(client->handle, handle, handleLen);

	return 0;
}

int initialPacketCheck(struct ClientInfo *client, int socketNum)
{
	uint8_t packet[MAXBUF];
	uint16_t packetSize = 3;

	memset(packet, 0, MAXBUF);

	setChatHeader(packet, packetSize, CONNECT_FLAG);
	setSender(packet + CHAT_HEADER_SIZE, client);

	//TODO Receive from the server the ACK
	safeSend(client->socket, packet, packetSize, 0);

	return getInitPacketResponse(client, socketNum);
}

int getInitPacketResponse(struct ClientInfo *client, int socketNum)
{
	char buf[CHAT_HEADER_SIZE];

	safeRecv(socketNum, buf, CHAT_HEADER_SIZE, 0);

	if (buf[2] == ACK_GOOD_FLAG) {
		printf("Confirmation packet recieved\n");
		return 0;
	} else if (buf[2] == ACK_BAD_FLAG) {
		handleInUse(client->handle);
		return -1;
	} else {
		printf("Unknown error\n");
	}

	return -1;
}

void sendToServer(int socketNum, struct ClientInfo *client)
{
	char inputBuf[MAXBUF];	//
	uint8_t packet[MAXBUF];  // Sending buffer
	int flag = 0;
	uint16_t sendLen = 3;        //amount of data to send - start as 3 for header
	int sent = 0;            //actual amount of data sent/* get the data and send it   */

	while (1)
	{
		// Don't need the returned socket - only would be the server
		if (pollCall(POLL_WAIT_FOREVER) != -1)
		{
			recvServer(socketNum);
		}

		memset(inputBuf, 0, MAXBUF);
		memset(packet, 0, MAXBUF);

		sendLen = getFromStdin(inputBuf, "\n$:");
		flag = parseInput(inputBuf, &sendLen, packet, client);

		if (flag != -1) {
			printf("read: %s string len: %d (including null)\n", inputBuf, sendLen);
		
			setChatHeader(packet, sendLen, flag);
			sent = safeSend(socketNum, packet, sendLen, 0);

			if (flag == GET_HANDLES_FLAG) {
				receiveHandleNumbers(socketNum);
			}

			printf("Amount of data sent is: %d\n", sent);

			// End the input loop
			if (flag == EXIT_FLAG) {
				break;
			}
		}
	}
}

void recvServer(int socketNum)
{
	char header[CHAT_HEADER_SIZE];
	char packet[MAXBUF];
	uint16_t packetLength = 0;
	uint8_t flag = 0;

	safeRecv(socketNum, header, CHAT_HEADER_SIZE, 0);

	packetLength = ntohs(*((uint16_t *) header));
	flag = header[2];

	safeRecv(socketNum, header, packetLength - CHAT_HEADER_SIZE, 0);

	parsePacket(packet, packetLength, flag);
}

void parsePacket(char *packet, uint16_t packetLength, uint8_t flag)
{
	switch (flag) {
		case BAD_DEST_FLAG:
			receivedBadDest(packet);
			break;

		case MESSAGE_FLAG:
			receivedMessage(packet, packetLength);
			break;

		case BROADCAST_FLAG:
			receivedBroadcast(packet, packetLength);
			break;

		default:
			printf("\nInvalid flag on packet received");
			break;
	}
}

void extractHandle(char *packet, char *handleBuff, uint8_t *handleLen)
{
	*handleLen = packet[0];

	memcpy(handleBuff, packet + 1, *handleLen);
	handleBuff[*handleLen] = '\0';
}

void receivedBadDest(char *packet)
{
	char handle[MAX_HANDLE_LENGTH];
	uint8_t handleLen = 0;

	extractHandle(packet, handle, &handleLen);
	handleNotFound(handle);
}

void receivedMessage(char *packet, uint16_t packetLength)
{
	char senderHandle[MAX_HANDLE_LENGTH];
	char messageBuff[MAX_MESSAGE_LENGTH];
	uint8_t handleLen = 0, numberHandles = 0, counter = 0;
	int packetOffset = 0;

	extractHandle(packet, senderHandle, &handleLen);
	packetOffset = handleLen + 1;

	numberHandles = packet[packetOffset];
	packetOffset++;

	while(counter < numberHandles) {
		handleLen = packet[packetOffset];
		packetOffset = packetOffset + handleLen + 1;
		counter++;
	}

	memcpy(messageBuff, packet + packetOffset, packetLength - packetOffset);
	printf("\n%s: %s", senderHandle, messageBuff);
}

void receivedBroadcast(char *packet, uint16_t packetLength)
{
	char senderHandle[MAX_HANDLE_LENGTH];
	char messageBuff[MAX_MESSAGE_LENGTH];
	int packetOffset = 0;
	uint8_t handleLen = 0;

	extractHandle(packet, senderHandle, &handleLen);

	packetOffset = handleLen + 1;

	memcpy(messageBuff, packet + packetOffset, packetLength - packetOffset);

	printf("\n%s: %s", senderHandle, messageBuff);
}

int parseInput(char *inputBuf, uint16_t *sendLen, uint8_t *packet, struct ClientInfo *client)
{
	char command[3];

	memcpy(command, inputBuf, 2);
	command[2] = '\0';

	// MESSAGE
	if (strcmp(command, "%M") == 0 || strcmp(command, "%m") == 0) {
		buildMessage(inputBuf + CHAT_HEADER_SIZE, sendLen, packet + CHAT_HEADER_SIZE, client);
		return MESSAGE_FLAG;
	}
	
	// BROADCAST
	else if (strcmp(command, "%B") == 0 || strcmp(command, "%b") == 0) {
		buildBroadcast(inputBuf + CHAT_HEADER_SIZE, sendLen, packet + CHAT_HEADER_SIZE, client);
		return BROADCAST_FLAG;
	}

	// LIST HANDLES
	else if (strcmp(command, "%L") == 0 || strcmp(command, "%l") == 0) {
		return GET_HANDLES_FLAG;
	}

	// EXIT
	else if (strcmp(command, "%E") == 0 || strcmp(command, "%e") == 0) {
		return EXIT_FLAG;
	}

	else {
		printf("\nInvalid command");
		return -1;
	}
}

void buildBroadcast(char *inputBuf, uint16_t *sendLen, uint8_t *packet, struct ClientInfo *client)
{
	// Handle size byte and handle size
	int packetOffset = client->handleLength + 1;

	// Set the sender
	setSender(packet, client);

	// Total size increase by client length and one client handle length byte
	*sendLen = *sendLen + packetOffset;

	// Add the message to the packet - Increase the index by one to consider the space
	addMessage(inputBuf + 1, sendLen, packet + packetOffset);
}

void buildMessage(char *inputBuf, uint16_t *sendLen, uint8_t *packet, struct ClientInfo *client)
{
	int handleLength = client->handleLength;
	char num[2] = {inputBuf[0], '\0'};
	uint8_t numHandles = atoi(num);

	// Set the sender
	setSender(packet, client);
	// Increment the packet by senderLength + 1 and set the number of handles
	*(packet + handleLength) = numHandles;
	
	// Total size increase by client length and two more for the number of handles and client handle lenght byte
	*sendLen = *sendLen + handleLength + 2;

	// Increment the input by two for the number and the space 
	// Increment the packet by the sender handleLength, + 2 bytes for numHandles and handleLength
	addHandles(inputBuf + 2, sendLen, packet + handleLength + 2, numHandles);
}

void addHandles(char *inputBuf, uint16_t *sendLen, uint8_t *packet, uint8_t numHandles)
{
	char handleBuf[MAX_HANDLE_LENGTH];
	char currentChar = 0;
	int idx = 0;
	uint8_t handleLen = 0;
	int inputIndex = 0, packetIndex = 0;

	while (idx < numHandles) {
		memset(inputBuf, '\0', MAX_HANDLE_LENGTH);

		while(isspace(currentChar = inputBuf[inputIndex]) == 0) {
			handleBuf[handleLen] = currentChar;
			handleLen++;
			inputIndex++;
		}

		packet[packetIndex++] = handleLen;
		packetIndex++;
		memcpy(packet + packetIndex, handleBuf, handleLen);
		packetIndex = packetIndex + handleLen;

		// Increment the total size by 1 byte for handleLength and the handleLength
		*sendLen = *sendLen + 1 + handleLen;

		// Increment for the space between the handles
		inputIndex++;
		handleLen = 0;

		idx++;
	}

	// Add the message to the packet - Increase the index by one to consider the space
	addMessage(inputBuf + inputIndex + 1, sendLen, packet + packetIndex);
}

void addMessage(char *inputBuf, uint16_t *sendLen, uint8_t *packet)
{
	char currentChar = 0;
	int messageLen = 0;

	while ((currentChar = inputBuf[messageLen]) != '\n') {
		packet[messageLen] = currentChar;
		messageLen++;
	}

	// Increment the total size by the message Length
	*sendLen = *sendLen + messageLen;
}

void setSender(uint8_t *packet, struct ClientInfo *client)
{
	packet[0] = client->handleLength;

	memcpy(packet + 1, client->handle, client->handleLength);
}

void receiveHandleNumbers(int socketNum)
{
	char packet[CHAT_HEADER_SIZE + 4];
	uint32_t numberHandles = 0;
	uint8_t flag = 0;

	safeRecv(socketNum, packet, CHAT_HEADER_SIZE + 4, 0);
	flag = packet[2];
	numberHandles = ntohl(*((uint32_t *) &(packet[3])));

	if (flag == NUM_HANDLES_FLAG) {
		printf("\nHandle List:");
		receiveHandles(socketNum, numberHandles);
	} else {
		printf("Did not receive the number of handles flag\n");
	}
}

void receiveHandles(int socketNum, uint32_t numberHandles)
{
	int handleCount = 0;
	uint8_t handleLength = 0, flag = 0;
	char header[CHAT_HEADER_SIZE + 1];
	char handle[MAX_HANDLE_LENGTH];

	while (handleCount < numberHandles) {
		safeRecv(socketNum, header, CHAT_HEADER_SIZE + 1, 0);
		flag = header[2];
		handleLength = header[3];
		
		if (flag != HANDLE_FLAG) {
			printf("Invalid Flag for handle packet");
		}

		safeRecv(socketNum, handle, handleLength, 0);
		printf("\n\t%d: %s", handleCount, handle);

		handleCount++;
	}

	allHandlesReceived(socketNum);
}

void allHandlesReceived(int socketNum)
{
	uint8_t flag = 0;
	char header[CHAT_HEADER_SIZE];

	safeRecv(socketNum, header, CHAT_HEADER_SIZE, 0);
	flag = header[2];

	if (flag != HANDLES_END_FLAG) {
		printf("Invalid Flag for handles end");
	} else {
		printf("\nEnd of handle list");
	}
}

int getFromStdin(char * sendBuf, char * prompt)
{
	// Gets input up to MAXBUF-1 (and then appends \0)
	// Returns length of string including null
	char aChar = 0;
	int inputLen = 0;       
	
	// Important you don't input more characters than you have space 
	printf("%s ", prompt);
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			sendBuf[inputLen] = aChar;
			inputLen++;
		}
	}
	// TODO input length too long

	sendBuf[inputLen] = '\0';
	inputLen++;  //we are going to send the null
	
	return inputLen;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 3)
	{
		printf("usage: %s host-name port-number \n", argv[0]);
		exit(1);
	}
}
