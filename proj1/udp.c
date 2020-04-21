#include "udp.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void getUDP(const uint8_t *packetData, int packetLength) {
   struct udpHeader *header = (struct udpHeader*) malloc(sizeof(struct udpHeader));

   header->SOURCE_PORT = ntohs(((u_int16_t *)packetData)[0]);
   header->DEST_PORT = ntohs(((u_int16_t *)packetData)[1]);

   printUDP(header);
   free(header);
}

void printUDP(struct udpHeader *header) {
   printf("\n\tUDP Header\n");
	printf("\t\tSource Port: : %d\n", header->SOURCE_PORT);
	printf("\t\tDest Port: : %d\n", header->DEST_PORT);
}