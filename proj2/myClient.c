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
#include "safeSystemUtil.h"

#define DEBUG_FLAG 1

struct ClientInfo {
	int socket;
	char handle[MAX_HANDLE_LENGTH];
	uint8_t handleLength;
};

void sendToServer(int socketNum);
void checkArgs(int argc, char * argv[]);
int getFromStdin(char * sendBuf, char * prompt);
int initClient(ClientInfo *client, int socketNum, char *handle);
int parseInput(char *inputBuf, uint16_t *sendLen, uint8_t *packet, ClientInfo *client);
void parseBroadcast(char *inputBuf, uint16_t *sendLen, uint8_t *packet, ClientInfo *client);
void parseNumHandles(char *inputBuf, uint16_t *sendLen, uint8_t *packet, ClientInfo *client);
void parseHandles(char *inputBuf, uint16_t *sendLen, uint8_t *packet, uint8_t numHandles);
void setSender(uint8_t *packet, ClientInfo *client);
void addMessage(char *inputBuf, uint16_t *sendLen, uint8_t *packet);

// TODO CHECK INCREMENTING ON PACKET AND INPUT
int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor
	ClientInfo client;
	
	checkArgs(argc, argv);

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[1], argv[2], DEBUG_FLAG);
	
	if (initClient(&client, socketNum, argv[2]) == 0) {
		if (initialPacketCheck(&client) == 0) {
			sendToServer(socketNum);
		}
	}
	
	safeClose(socketNum);
	
	return 0;
}

int initClient(ClientInfo *client, int socketNum, char *handle)
{
	uint8_t handleLen = 0;

	client->socket = socketNum;
	memset(client->handle, '\0', MAX_HANDLE_LENGHTH);

	// Check handle length
    if ((handleLen = strlen(handle)) >  MAX_HANDLE_LENGTH) {
    	handleTooLong(handle);
    	return -1;
    }

	client->handleLength = handleLen;

	strncpy(client->handle, handle, MAX_HANDLE_LENGHTH);

	return 0;
}

void initialPacketCheck(ClientInfo *client) {
	int sent = 0;            //actual amount of data sent/* get the data and send it   */
	uint8_t packet[MAXBUF];
	uint16_t packetSize = 3;

	memset(packet, 0, MAXBUF);

	setSender(packet + CHAT_HEADER_SIZE, &packetSize, client);
	setChatHeader(packet, packetSize, CONNECT_FLAG);

	//TODO Receive from the server the ACK
	sent = safeSend(client->socket, packet, packetSize, 0);
}

void sendToServer(int socketNum, ClientInfo *client)
{
	char inputBuf[MAXBUF];	//
	uint8_t packet[MAXBUF];  // Sending buffer
	int command = 0;
	uint16_t sendLen = 3,;        //amount of data to send - start as 3 for header
	int sent = 0;            //actual amount of data sent/* get the data and send it   */
	
	while (1)
	{
		memset(inputBuf, 0, MAXBUF);
		memset(packet, 0, MAXBUF); 

		sendLen = getFromStdin(inputBuf, "$:");
		command = parseInput(inputBuf, &sendLen, packet, client);

		if (command != -1) {
			printf("read: %s string len: %d (including null)\n", inputBuf, sendLen);
		
			((uint16_t *) packet)[0] = htons(sendLen);
			sent =  safeSend(socketNum, packet, sendLen, 0);

			printf("Amount of data sent is: %d\n", sent);

			// End the input loop
			if (command == EXIT_FLAG) {
				break;
			}
		}
	}
}

int parseInput(char *inputBuf, uint16_t *sendLen, uint8_t *packet, ClientInfo *client)
{
	char command[3];

	memcpy(command, inputBuf, 2);
	command[2] = '\0'

	// MESSAGE
	if (strcmp(command, "%M") == 0 || strcmp(command, "%m") == 0) {
		packet[2] = MESSAGE_FLAG;
		parseNumHandles(inputBuf + CHAT_HEADER_SIZE, sendLen, packet + CHAT_HEADER_SIZE, client);                                                                                                    )
	}
	
	// BROADCAST
	else if (strcmp(command, "%B") == 0 || strcmp(command, "%b") == 0) {
		packet[2] = BROADCAST_FLAG;
		parseBroadcast(inputBuf + CHAT_HEADER_SIZE, sendLen, packet + CHAT_HEADER_SIZE, client);
	}

	// LIST HANDLES
	else if (strcmp(command, "%L") == 0 || strcmp(command, "%l") == 0) {
		// LIST HANDLES
		packet[2] = GET_HANDLES_FLAG;
		return GET_HANDLES_FLAG;
	}

	// EXIT
	else if (strcmp(command, "%E") == 0 || strcmp(command, "%e") == 0) {
		packet[2] = GET_HANDLES_FLAG;
		return EXIT_FLAG;
	}

	else {
		printf("\nInvalid command");
		return -1;
	}
	
	return 0;
}

void parseBroadcast(char *inputBuf, uint16_t *sendLen, uint8_t *packet, ClientInfo *client)
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

void parseNumHandles(char *inputBuf, uint16_t *sendLen, uint8_t *packet, ClientInfo *client)
{
	int handleLength = client->handleLength;
	uint8_t numHandles = atoi(inputBuf[0]);

	// Set the sender
	setSender(packet, client);
	// Increment the packet by senderLength + 1 and set the number of handles
	*(packet + handleLength) = numHandles;
	
	// Total size increase by client length and two more for the number of handles and client handle lenght byte
	*sendLen = *sendLen + handleLength + 2;

	// Increment the input by two for the number and the space 
	// Increment the packet by the sender handleLength
	parseHandles(inputBuf + 2, sendLen, packet + handleLength + 1);
}

void parseHandles(char *inputBuf, uint16_t *sendLen, uint8_t *packet, uint8_t numHandles)
{
	char handleBuf[MAX_HANDLE_LENGHTH];
	char currentChar = 0;
	int idx = 0;
	uint8_t handleLen = 0;
	int inputIndex = 0, packetIndex = 0;

	while (idx < numHandles) {
		memset(inputBuf, '\0', MAX_HANDLE_LENGHTH);

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

void setSender(uint8_t *packet, ClientInfo *client)
{
	packet[0] = client->handleLength;

	memcpy(packet + 1, client->handle, client->handleLength);
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
