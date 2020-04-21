#include "icmp.h"
#include <stdlib.h>
#include <stdio.h>

void getICMP(const uint8_t *packetData, int packetLength) {
   struct icmpHeader *header = (struct icmpHeader*) malloc(sizeof(struct icmpHeader));

   header->type = *(packetData);

   printICMP(header);
   free(header);
}

void printICMP(struct icmpHeader *header) {
   printf("\n\tICMP Header\n");
	printf("\t\tType: %s\n", header->type == ICMP_REPLY ? "Reply" : header->type == ICMP_REQUEST ? "Request" : "Unknown");
}