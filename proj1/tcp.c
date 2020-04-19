#include "tcp.h"

void tcp() {
   //
}

void printTCP(struct tcpHeader *header) {
   printf("\n\tTCP Header\n");
	printf("\t\tSource Port: : %d\n", sourcePort);
	printf("\t\tDest Port: : %d\n", destPort);
	printf("\t\tSequence Number: %d\n", seqNum);
	printf("\t\tACK Number: %d\n", ackNum);
	printf("\t\tACK Flag: %s\n", ackFlag ? "Yes" : "No");
	printf("\t\tSYN Flag: %s\n", synFlag ? "Yes" : "No");
	printf("\t\tRST Flag: %s\n", rstFlag ? "Yes" : "No");
	printf("\t\tFIN Flag: %s\n", finFlag ? "Yes" : "No");
	printf("\t\tWindow Size: %d\n", windowSize);
	printf("\t\tChecksum: %s (%x)\n", goodCheck ? "Correct" : "Incorrect", checkSumVal);
}