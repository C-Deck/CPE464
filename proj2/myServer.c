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
#include "list.h"

#define DEBUG_FLAG 1

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
void processSockets(int mainServerSocket);
void setHandle(int socketNum, char *packet);
void initClientList();
void parseHeader(int clientSocket, char *packet);
void doCommand(int socketNum, char *packet, uint16_t packetSize, uint8_t flag);
void addNewClient(int mainServerSocket);
void removeClient(int clientSocket);

ClientList *clientList;

int main(int argc, char *argv[])
{
	int serverSocket = 0;   //socket descriptor for the server socket
	int clientSocket = 0;   //socket descriptor for the client socket
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	initClientList();
	
	//create the server socket
	serverSocket = tcpServerSetup(portNumber);

	// wait for client to connect - select version
	// clientSocket = tcpAccept(serverSocket, DEBUG_FLAG);

	// Main control process (clients and accept())
	processSockets(serverSocket);
	
	/* close the sockets */
	safeClose(serverSocket);

	return 0;
}

void initClientList()
{
	clientList = (ClientList *) safeMalloc(sizeof(ClientList));
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
	
	// Use a time value of 1 second (so time is not null)
	while (selectCall(clientSocket, 1, 0, TIME_IS_NOT_NULL) == 0)
	{
		printf("Select timed out waiting for client to send data\n");
	}
	
	//now get the data from the client_socket (message includes null)
	messageLen = safeRecv(clientSocket, buf, MAXBUF, 0);
	if (message == 0) {
		// recv() 0 bytes so client is gone
		removeClient(clientSocket);
	}

	parseHeader(clientSocket, buf);
}

void parseHeader(int clientSocket, char *packet)
{
	uint16_t packetSize = ntohs(*((uint16_t *) packet));
	uint8_t flag = packet[2];

	doCommand(packet, packetSize, flag);
}

void doCommand(int socketNum, char *packet, uint16_t packetSize, uint8_t flag)
{
	switch (flag) {
		case CONNECT_FLAG:
			setHandle(socketNum, packet + CHAT_HEADER_SIZE);
			break;
		case MESSAGE_FLAG:
			sendMessage(packet);
			break;
		case BROADCAST_FLAG:
			sendBroadcast(packet);
			break;
		case GET_HANDLES_FLAG:
			sendHandleLength(socketNum);
			break;
		case EXIT_FLAG:
			exitClient(socketNum);
			break;
		default:
			printf("\nInvalid flag in header");
			break;
	}
}

void setHandle(int socketNum, char *packet)
{
	char sendPacket[3];
	char handleBuf[MAX_HANDLE_LENGHTH];
	uint8_t handleSize = packet[0];
	
	strncpy(handleBuf, packet + 1, handleSize);
	handleBuf[handleSize] = '\0';

	if (checkHandleExists(clientList, handleBuf) == 0) {
		setChatHeader(sendPacket, 3, ACK_GOOD_FLAG);
		// Send the packet
	} else {
		setChatHeader(sendPacket, 3, ACK_BAD_FLAG);
		// Send the packet
	}
}

void sendHandleLength(int socketNum)
{
	char packet[7];
	uint16_t packetLength = 7;
	uint32_t numHandles = 0;

	setChatHeader(packet, packetLength, NUM_HANDLES_FLAG);
	numHandles = clientList->numClients;
	*((uint32_t *) (packet + CHAT_HEADER_SIZE)) = numHandles;

	// Send the packet


	sendAllHandles(socketNum);
}

void sendHandlePacket(int socketNum, char *handle, uint8_t handleLength)
{
	char packet[MAX_HANDLE_LENGHTH + 4];
	int packetSize = CHAT_HEADER_SIZE + handleLength + 1;

	setChatHeader(packet, packetSize, HANDLE_FLAG);
	packet[CHAT_HEADER_SIZE] = handleLength;

	memcpy(packet + CHAT_HEADER_SIZE + 1, handle, handleLength);

	// Send the packet
}

void sendHandleListFinished(int socketNum)
{
	char packet[3];

	setChatHeader(packet, 3, HANDLES_END_FLAG);
	// Send the packet
	
}

void addNewClient(int mainServerSocket)
{
	int newClientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);

	addToPollSet(newClientSocket);
}

void removeClient(int clientSocket)
{
	printf("Client on socket %d terminted\n", clientSocket);
	removeFromPollSet(clientSocket);
	safeClose(clientSocket);
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

