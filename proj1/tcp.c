#include "tcp.h"
#include "checksum.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>

void getTCP(const uint8_t *packetData, int tcp_size, uint8_t *psuedoHeader) {
	struct tcpHeader *header = (struct tcpHeader*) malloc(sizeof(struct tcpHeader));
    uint16_t checksum;
	int byteAdjustment = 0;

	/* SOURCE PORT*/
	header->SOURCE_PORT = ntohs(*((uint16_t *) packetData));
	byteAdjustment = byteAdjustment + 2;

	/* DESTINATION PORT*/
    header->DEST_PORT = ntohs(*((uint16_t *) (packetData + byteAdjustment)));
	byteAdjustment = byteAdjustment + 2;

	/* Sequence Number */
    header->SEQ_NUM = ntohl(*((uint32_t *) (packetData + byteAdjustment)));
    byteAdjustment = byteAdjustment + 4;

    /* Acknowledgement Number */
    header->ACK_NUM = ntohl(*((uint32_t *) (packetData + byteAdjustment)));
    byteAdjustment = byteAdjustment + 4 + 1; /* Add one more for offset to get to flags */

	/* Flags byte */
	header->FLAGS = *(packetData + byteAdjustment);
	byteAdjustment++;

	/* Window */
    header->WINDOW = ntohs(*((uint16_t *) (packetData + byteAdjustment)));
    byteAdjustment = byteAdjustment + 2;

	/* CHECKSUM */
    header->CHECKSUM = ntohs(*((uint16_t *) (packetData + byteAdjustment)));
    byteAdjustment = byteAdjustment + 2;

	memcpy(psuedoHeader + 12, packetData, tcp_size);

	checksum = in_cksum((uint16_t *) (psuedoHeader + 12), tcp_size);

	printTCP(header, checksum);
	free(header);
}

void printTCP(struct tcpHeader *header, uint16_t checksum) {
    printf("\n\tTCP Header\n");

	if (header->SOURCE_PORT == HTTP) {
		printf("\t\tSource Port:  HTTP\n");
	} else {
		printf("\t\tSource Port: : %d\n", header->SOURCE_PORT);
	}

	if (header->DEST_PORT == HTTP) {
		printf("\t\tDest Port:  HTTP\n");
	} else {
		printf("\t\tDest Port: : %d\n", header->DEST_PORT);
	}

	printf("\t\tSequence Number: %u\n", header->SEQ_NUM);
	if ((header->FLAGS & ACK_MASK) == ACK_MASK) {
		printf("\t\tACK Number: %u\n", header->ACK_NUM);
	} else {
		printf("\t\tACK Number: <not valid>\n");
	}
	
	printf("\t\tACK Flag: %s\n", ((header->FLAGS & ACK_MASK) == ACK_MASK) ? "Yes" : "No");
	printf("\t\tSYN Flag: %s\n", ((header->FLAGS & SYN_MASK) == SYN_MASK) ? "Yes" : "No");
	printf("\t\tRST Flag: %s\n", ((header->FLAGS & RST_MASK) == RST_MASK) ? "Yes" : "No");
	printf("\t\tFIN Flag: %s\n", ((header->FLAGS & FIN_MASK) == FIN_MASK) ? "Yes" : "No");
	printf("\t\tWindow Size: %d\n", header->WINDOW);
	printf("\t\tChecksum: %s (0x%x)\n", checksum ? "Correct" : "Incorrect", header->CHECKSUM);
}