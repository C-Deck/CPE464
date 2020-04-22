#ifndef IP_H
#define IP_H

#include <arpa/inet.h>
#include "tcp.h"

#define ICMP_PROTOCOL 1
#define TCP_PROTOCOL 6
#define UDP_PROTOCOL 17

#define IP_LENGTH 4
#define WORD_SIZE 4

struct ipHeader {
	uint8_t HDR;						/* Hardware length          - 1 byte */
	uint8_t TOS;			  			/* Type of Service 			- 2 bytes */
	uint16_t TL;						/* Total length				- 2 bytes */
	uint16_t ID;						/* Identification			- 2 bytes */
	uint16_t FLAGS;						/* Flags and frag offset	- 2 bytes */
	uint8_t TTL;						/* Time to Live				- 1 byte */
	uint8_t PROTOCOL;					/* Protocol					- 1 byte */
	uint16_t HEADER_CHECKSUM;			/* Header Checksum			- 2 bytes */
	uint32_t SOURCE_ADDR;				/* Source IP Address		- 4 bytes */
	uint32_t DEST_ADDR;					/* Destination IP Address	- 4 bytes */
} __attribute__((packed));

void getIP(const uint8_t *packetData, int packetLength);
int pseudoHeader(uint8_t *pseudoHeader, struct ipHeader *header);
void seudoHeader(struct PseudoHeader *pseudoHeader, struct ipHeader *header);
void printIP(struct ipHeader *header, u_int16_t checksum);
char *printProtocol(struct ipHeader *header);

#endif