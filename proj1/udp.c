#include "udp.h"

void udp() {
   //
}

void printUDP(struct udpHeader *header) {
   printf("\n\tUDP Header\n");
	printf("\t\tSource Port: : %d\n", header->src);
	printf("\t\tDest Port: : %d\n", header->dest);
}