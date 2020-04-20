#include "icmp.h"

void icmp(uint8_t *packetData, int packetLength) {
   //
}

void printICMP(struct icmpHeader *header) {
   printf("\n\tICMP Header\n");
	printf("\t\tType: %sn", header->type == ICMP_REPLY ? "Reply" : header->type == ICMP_REQUEST ? "Request" : "Unknown");
}