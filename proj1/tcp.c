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
	header->FLAGS = ntohs(*(packetData + byteAdjustment));
	byteAdjustment++;

	/* Window */
    header->WINDOW = ntohs(*((uint16_t *) (packetData + byteAdjustment)));
    byteAdjustment = byteAdjustment + 2;

	/* CHECKSUM */
    header->CHECKSUM = ntohs(*((uint16_t *) (packetData + byteAdjustment)));
    byteAdjustment = byteAdjustment + 2;

	checksum = in_cksum((uint16_t *) (psuedoHeader + 12), tcp_size + 12);

	printTCP(header, checksum);
	free(header);
}

void printTCP(struct tcpHeader *header, uint16_t checksum) {
   printf("\n\tTCP Header\n");
	printf("\t\tSource Port: : %d\n", header->SOURCE_PORT);
	printf("\t\tDest Port: : %d\n", header->DEST_PORT);
	printf("\t\tSequence Number: %d\n", header->SEQ_NUM);
	printf("\t\tACK Number: %d\n", header->ACK_NUM);
	printf("\t\tACK Flag: %s\n", (header->FLAGS & ACK_MASK) ? "Yes" : "No");
	printf("\t\tSYN Flag: %s\n", (header->FLAGS & SYN_MASK) ? "Yes" : "No");
	printf("\t\tRST Flag: %s\n", (header->FLAGS & RST_MASK) ? "Yes" : "No");
	printf("\t\tFIN Flag: %s\n", (header->FLAGS & FIN_MASK) ? "Yes" : "No");
	printf("\t\tWindow Size: %d\n", header->WINDOW);
	printf("\t\tChecksum: %s (0x%x)\n", checksum ? "Correct" : "Incorrect", header->CHECKSUM);
}