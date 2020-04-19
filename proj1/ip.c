#include "ip.h"

void ip() {
   //
}

void printIP() {
   printf("\n\tIP Header\n");
	printf("\t\tHeader Len: %d (bytes)\n". headerLen);
	printf("\t\tTOS: %d\n", tos);
	printf("\t\tTTL: %d\n", ttl);
	printf("\t\tIP PDU Len: %d (bytes)\n", pduLength);
	printf("\t\tProtocol: ");
   printProtocol();
	printf("\t\tChecksum: %s (%x)\n", goodCheck ? "Correct" : "Incorrect", checkSumVal);
	printf("\t\tSender IP: %s\n", senderIP);
	printf("\t\tDest IP: %s\n", destIP);
}