
/* 	Code originally give to Prof. Smith by his TA in 1994.
	No idea who wrote it.  Copy and use at your own Risk
*/


#ifndef __NETWORKS_H__
#define __NETWORKS_H__

#include <stdint.h>

#define BACKLOG 10
#define MAXBUF 1024
#define TIME_IS_NULL 1
#define TIME_IS_NOT_NULL 2
#define MAX_HANDLE_LENGHTH 100
#define CHAT_HEADER_SIZE 3

// Client side send flags - received by server
#define CONNECT_FLAG 0x1
#define MESSAGE_FLAG 0x5
#define BROADCAST_FLAG 0x4
#define GET_HANDLES_FLAG 0xA
#define EXIT_FLAG 0x8

// Server side send flags - received by client
#define ACK_GOOD_FLAG 0x2
#define ACK_BAD_FLAG 0x3
#define BAD_DEST_FLAG 0x7
#define ACK_EXIT_FLAG 0x9
#define NUM_HANDLES_FLAG 0xB
#define HANDLE_FLAG 0xC
#define HANDLES_END_FLAG 0xD

// for the server side
int tcpServerSetup(int portNumber);
int tcpAccept(int server_socket, int debugFlag);

// for the client side
int tcpClientSetup(char * serverName, char * port, int debugFlag);

int selectCall(int socketNumber, int seconds, int microseconds, int timeIsNotNull);

void setChatHeader(char *packet, uint16_t packetLength, uint8_t flag);

#endif
