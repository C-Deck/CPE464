
// Hugh Smith April 2017
// Network code to support TCP client server connections

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
#include "gethostbyname6.h"
#include "safeSystemUtil.h"

// This function creates the server socket.  The function 
// returns the server socket number and prints the port 
// number to the screen.

int tcpServerSetup(int portNumber)
{
	int server_socket= 0;
	struct sockaddr_in6 server;      /* socket address for local side  */
	socklen_t len= sizeof(server);  /* length of local address        */

	/* create the tcp socket  */
	server_socket = safeSocket(AF_INET6, SOCK_STREAM, 0);

	// setup the information to name the socket
	server.sin6_family= AF_INET;         		
	server.sin6_addr = in6addr_any;   //wild card machine address
	server.sin6_port= htons(portNumber);         

	// bind the name to the socket  (name the socket)
	safeBind(server_socket, (struct sockaddr *) &server, sizeof(server));
	
	//get the port number and print it out
	if (getsockname(server_socket, (struct sockaddr*)&server, &len) < 0)
	{
		perror("getsockname call");
		exit(-1);
	}

	safeListen(server_socket, BACKLOG);
	
	printf("Server Port Number %d \n", ntohs(server.sin6_port));
	
	return server_socket;
}

// This function waits for a client to ask for services.  It returns
// the client socket number.   

int tcpAccept(int server_socket, int debugFlag)
{
	struct sockaddr_in6 clientInfo;   
	int clientInfoSize = sizeof(clientInfo);
	int client_socket= 0;

	client_socket = safeAccept(server_socket, (struct sockaddr*) &clientInfo, (socklen_t *) &clientInfoSize);
	
	if (debugFlag)
	{
		printf("Client accepted.  Client IP: %s Client Port Number: %d\n",  
				getIPAddressString(clientInfo.sin6_addr.s6_addr), ntohs(clientInfo.sin6_port));
	}

	return(client_socket);
}

int tcpClientSetup(char * serverName, char * port, int debugFlag)
{
	// This is used by the client to connect to a server using TCP
	
	int socket_num;
	uint8_t * ipAddress = NULL;
	struct sockaddr_in6 server;      
	
	// create the socket
	socket_num = safeSocket(AF_INET6, SOCK_STREAM, 0);
	
	if (debugFlag)
	{
		printf("Connecting to server on port number %s\n", port);
	}
	
	// setup the server structure
	server.sin6_family = AF_INET6;
	server.sin6_port = htons(atoi(port));
	
	// get the address of the server 
	if ((ipAddress = getIPAddress6(serverName, &server)) == NULL)
	{
		exit(-1);
	}

	printf("server ip address: %s\n", getIPAddressString(ipAddress));
	safeConnect(socket_num, (struct sockaddr*)&server, sizeof(server));

	if (debugFlag)
	{
		printf("Connected to %s IP: %s Port Number: %d\n", serverName, getIPAddressString(ipAddress), atoi(port));
	}
	
	return socket_num;
}

void setChatHeader(uint8_t *packet, uint16_t packetLength, uint8_t flag)
{
	((uint16_t *) packet)[0] = htons(packetLength);
	packet[2] = NUM_HANDLES_FLAG;
}