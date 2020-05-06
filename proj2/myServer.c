/******************************************************************************
* tcp_server.c
*
* CPE 464 - Program 1
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

#include "networks.h"
#include "safeSystemUtil.h"
#include "pollLib.h"
#include "list.h"

#define DEBUG_FLAG 1
#define NORMAL_MODE 1
#define DEBUG_MODE 2

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
void processSockets(int mainServerSocket);
void setHandle(int socketNum, char *packet);
void initClientList();
void parseHeader(int clientSocket, char *packet);
void doCommand(int socketNum, char *packet, uint16_t packetSize, uint8_t flag);
void addNewClient(int mainServerSocket);
void removeClient(int clientSocket);
void sendMessage(char *packet, int senderSocket, uint16_t packetSize);
void attemptSendMessage(uint8_t handleLength, char *handle, char *packet, int senderSocket, uint16_t packetSize);
void broadcast(int socketNum, char *packet, uint16_t packetSize);
void broadcastToClient(int socketNum, char *packet, uint16_t packetSize);
void sendAllHandles(int socketNum);
void sendHandleFlag(int socketNum, char *handle, uint8_t handleLength);
void sendHandlePacket(int socketNum, char *handle, uint8_t handleLength, uint8_t flag);
void sendHandleListFinished(int socketNum);
void exitClient(int socketNum);

struct ClientList *clientList;
static int currentMode = DEBUG_MODE;

int main(int argc, char *argv[])
{
	int serverSocket = 0;   //socket descriptor for the server socket
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	initClientList();
	
	//create the server socket
	serverSocket = tcpServerSetup(portNumber);

	// Main control process (clients and accept())
	processSockets(serverSocket);

	free(clientList);
	
	/* close the sockets */
	safeClose(serverSocket);

	return 0;
}

void initClientList()
{
	if (currentMode == DEBUG_MODE) {
		printf("\nInitialized the client list");
	}

	clientList = (struct ClientList *) safeMalloc(sizeof(struct ClientList));
	clientList->head = NULL;
	clientList->tail = NULL;
	clientList->numClients = 0;
}

void processSockets(int mainServerSocket)
{
	int socketToProcess = 0;
	
	addToPollSet(mainServerSocket);
		
	while (1)
	{
		if ((socketToProcess = pollCall(POLL_WAIT_FOREVER)) != -1)
		{
			if (socketToProcess == mainServerSocket)
			{
				addNewClient(mainServerSocket);
			}
			else
			{
				recvFromClient(socketToProcess);
			}
		}
		else
		{
			// Just printing here to let me know what is going on
			printf("Poll timed out waiting for client to send data\n");
		}
		
	}
}

void recvFromClient(int clientSocket)
{
	char buf[MAXBUF];
	int messageLen = 0;
	
	//now get the data from the client_socket (message includes null)
	//messageLen = safeRecv(clientSocket, buf, MAXBUF, 0);
	if ((messageLen = recv(clientSocket, buf, MAXBUF, 0)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

	if (currentMode == DEBUG_MODE) {
		struct Client *client = getClientFromSocket(clientList, clientSocket);
		printf("\nReceived %d bytes from socket %s", messageLen, client->handle);
	}

	if (messageLen == 0) {
		// recv() 0 bytes so client is gone
		removeClient(clientSocket);
	}

	parseHeader(clientSocket, buf);
}

void parseHeader(int clientSocket, char *packet)
{
	uint16_t packetSize = ntohs(*((uint16_t *) packet));
	uint8_t flag = (uint8_t) packet[2];

	if (currentMode == DEBUG_MODE) {
		printf("\nParsed the header -- PacketSize: %d - Flag: %d", packetSize, flag);
	}

	doCommand(clientSocket, packet, packetSize, flag);
}

void doCommand(int socketNum, char *packet, uint16_t packetSize, uint8_t flag)
{
	switch (flag) {
		case CONNECT_FLAG:
			setHandle(socketNum, packet + CHAT_HEADER_SIZE);
			break;
		case MESSAGE_FLAG:
			sendMessage(packet, socketNum, packetSize);
			break;
		case BROADCAST_FLAG:
			if (currentMode == DEBUG_MODE) {
				printf("\nMessage handling broadcast flag");
			}
			broadcast(socketNum, packet, packetSize);
			//forEachWithPacket(clientList, broadcastToClient, packet, packetSize, socketNum);
			break;
		case GET_HANDLES_FLAG:
			sendAllHandles(socketNum);
			break;
		case EXIT_FLAG:
			exitClient(socketNum);
			break;
		default:
			printf("\nInvalid flag in header");
			break;
	}
}

void sendMessage(char *packet, int senderSocket, uint16_t packetSize)
{
	uint8_t numClients = 0, currentHandleLength = 0, offset = CHAT_HEADER_SIZE, idx = 0;
	char handle[MAX_HANDLE_LENGTH];

	currentHandleLength = *(packet + CHAT_HEADER_SIZE);
	offset = currentHandleLength + 1;

	numClients = *(packet + offset);
	offset++;

	if (currentMode == DEBUG_MODE) {
		printf("\nMessage being sent to %d clients", numClients);
	}

	while (idx < numClients) {
		memset(handle, '\0', MAX_HANDLE_LENGTH);

		currentHandleLength = *(packet + offset);
		offset++;

		memcpy(handle, packet + offset, currentHandleLength);
		offset = offset + currentHandleLength;

		attemptSendMessage(currentHandleLength, handle, packet, senderSocket, packetSize);
	
		idx++;
	}
}

void attemptSendMessage(uint8_t handleLength, char *handle, char *packet, int senderSocket, uint16_t packetSize)
{
	struct Client *client = getClient(clientList, handle);

	if (client != NULL) {
		if (currentMode == DEBUG_MODE) {
			printf("\nClient <%s> found and sending packet size: %d", handle, packetSize);
		}

		// Send the packet
		//safeSend(client->socket, packet, packetSize, 0);
		if ((send(client->socket, packet, packetSize, 0)) < 0)
		{
			perror("send call");
			exit(-1);
		}
	} else {
		sendHandlePacket(senderSocket, handle, handleLength, BAD_DEST_FLAG);
	}
}

void setHandle(int socketNum, char *packet)
{
	uint8_t sendPacket[CHAT_HEADER_SIZE];
	char handleBuf[MAX_HANDLE_LENGTH];
	uint8_t handleSize = (uint8_t) packet[0];
	
	strncpy(handleBuf, packet + 1, handleSize);
	handleBuf[handleSize] = '\0';

	if (currentMode == DEBUG_MODE) {
		printf("\nHandle size of %d", handleSize);
		printf("\nTrying to set up connection for %s using packet", packet + 1);
		printf("\nTrying to set up connection for %s using handleBuf", handleBuf);
	}

	if (checkHandleExists(clientList, handleBuf) == 0) {
		setChatHeader(sendPacket, CHAT_HEADER_SIZE, ACK_GOOD_FLAG);

		// Send the packet
		//safeSend(socketNum, packet, CHAT_HEADER_SIZE, 0);
		if (currentMode == DEBUG_MODE) {
			printf("\nSending Successful connection");
		}
		if ((send(socketNum, packet, CHAT_HEADER_SIZE, 0)) < 0)
		{
			perror("send call");
			exit(-1);
		}
	} else {
		setChatHeader(sendPacket, CHAT_HEADER_SIZE, ACK_BAD_FLAG);

		if (currentMode == DEBUG_MODE) {
			printf("\nConnection by socket failed");
		}

		// Send the packet
		//safeSend(socketNum, packet, CHAT_HEADER_SIZE, 0);
		if ((send(socketNum, packet, CHAT_HEADER_SIZE, 0)) < 0)
		{
			perror("send call");
			exit(-1);
		}
	}
}

void broadcast(int socketNum, char *packet, uint16_t packetSize)
{
	struct Client *client = clientList->head;
	while (client != NULL) {
		if (client->handleSet == 1 && client->socket != socketNum) {
			broadcastToClient(client->socket, packet, packetSize);
		}

		client = client->nextClient;
	}
}

void broadcastToClient(int socketNum, char *packet, uint16_t packetSize) // Can be replaced with just a send
{
	if (currentMode == DEBUG_MODE) {
		uint8_t handleSize = packet[3];

		printf("\nBroadcasting message to socket %d", socketNum);
		printf("\nMessage: %s", packet + 4 + handleSize);
	}

	// Send the packet
	//safeSend(socketNum, packet, packetSize, 0);
	/* if ((send(socketNum, packet, packetSize, 0)) < 0)
	{
		perror("send call");
		exit(-1);
	} */
}

void sendAllHandles(int socketNum)
{
	char packet[CHAT_HEADER_SIZE + 4];
	uint16_t packetLength = CHAT_HEADER_SIZE + 4;
	uint32_t numHandles = 0;

	memset(packet, 0, CHAT_HEADER_SIZE + 4);

	setChatHeader((uint8_t *) packet, packetLength, NUM_HANDLES_FLAG);
	numHandles = clientList->numClients;
	*((uint32_t *) (packet + CHAT_HEADER_SIZE)) = htonl(numHandles);

	if (currentMode == DEBUG_MODE) {
		printf("\nNumber of handles: %d", numHandles);
	}

	// Send the packet
	//safeSend(socketNum, packet, CHAT_HEADER_SIZE + 4, 0);
	if ((send(socketNum, packet, CHAT_HEADER_SIZE + 4, 0)) < 0)
	{
		perror("send call");
		exit(-1);
	}

	forEachWithSender(clientList, sendHandleFlag, socketNum);

	sendAllHandles(socketNum);
}

// Use sendHandlePacket with flag preset -
void sendHandleFlag(int socketNum, char *handle, uint8_t handleLength)
{
	sendHandlePacket(socketNum, handle, handleLength, HANDLE_FLAG);
}

// Used for both flags 7 and 12
void sendHandlePacket(int socketNum, char *handle, uint8_t handleLength, uint8_t flag)
{
	char packet[MAX_HANDLE_LENGTH + CHAT_HEADER_SIZE + 1];
	int packetSize = CHAT_HEADER_SIZE + handleLength + 1;

	memset(packet, 0, MAX_HANDLE_LENGTH + CHAT_HEADER_SIZE + 1);

	setChatHeader((uint8_t *) packet, packetSize, flag);
	packet[CHAT_HEADER_SIZE] = handleLength;

	memcpy(packet + CHAT_HEADER_SIZE + 1, handle, handleLength);

	if (currentMode == DEBUG_MODE) {
		printf("\nSending handle <%s> to socket: %d - flag %d", handle, socketNum, flag);
	}

	// Send the packet
	//safeSend(socketNum, packet, packetSize, 0);
	if ((send(socketNum, packet, packetSize, 0)) < 0)
	{
		perror("send call");
		exit(-1);
	}
}

void sendHandleListFinished(int socketNum)
{
	char packet[CHAT_HEADER_SIZE];

	setChatHeader((uint8_t *)packet, CHAT_HEADER_SIZE, HANDLES_END_FLAG);

	if (currentMode == DEBUG_MODE) {
		printf("\nSending flag 13");
	}

	// Send the packet
	//safeSend(socketNum, packet, CHAT_HEADER_SIZE, 0);
	if ((send(socketNum, packet, CHAT_HEADER_SIZE, 0)) < 0)
	{
		perror("send call");
		exit(-1);
	}
}

void exitClient(int socketNum)
{
	char packet[CHAT_HEADER_SIZE];

	setChatHeader((uint8_t *) packet, CHAT_HEADER_SIZE, ACK_EXIT_FLAG);

	if (currentMode == DEBUG_MODE) {
		printf("\nSending exit acknowledge flag");
	}

	// Send the packet
	safeSend(socketNum, packet, CHAT_HEADER_SIZE, 0);
	if ((send(socketNum, packet, CHAT_HEADER_SIZE, 0)) < 0)
	{
		perror("send call");
		exit(-1);
	}
	
	removeClient(socketNum);
}

void addNewClient(int mainServerSocket)
{
	int newClientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);
	if (currentMode == DEBUG_MODE) {
		printf("\nAttempting to add the client to the list");
	}
	addClient(clientList, newClientSocket);
	if (currentMode == DEBUG_MODE) {
		printf("\nClient added to the list");
	}

	addToPollSet(newClientSocket);
}

void removeClient(int clientSocket)
{
	printf("Client on socket %d terminted\n", clientSocket);
	removeClientFromList(clientList, clientSocket);
	removeFromPollSet(clientSocket);
	//safeClose(clientSocket);
	close(clientSocket);
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

