#include "tcp.h"

void getTCP(uint8_t *packetData, int tcp_size, uint8_t *psuedoHeader) {
   //
}

void printTCP(struct tcpHeader *header, uint16_t checksum) {
   printf("\n\tTCP Header\n");
	printf("\t\tSource Port: : %d\n", header->SRC_PORT);
	printf("\t\tDest Port: : %d\n", header->DEST_PORT);
	printf("\t\tSequence Number: %d\n", header->SEQ_NUM);
	printf("\t\tACK Number: %d\n", header->ACK_NUM);
	printf("\t\tACK Flag: %s\n", header->ACK ? "Yes" : "No");
	printf("\t\tSYN Flag: %s\n", header->SYN ? "Yes" : "No");
	printf("\t\tRST Flag: %s\n", header->RST ? "Yes" : "No");
	printf("\t\tFIN Flag: %s\n", header->FIN ? "Yes" : "No");
	printf("\t\tWindow Size: %d\n", header->WINDOW);
	printf("\t\tChecksum: %s (%x)\n", checksum == header->CHECKSUM ? "Correct" : "Incorrect", header->CHECKSUM);
}