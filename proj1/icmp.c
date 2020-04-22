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
   if (header->type == ICMP_REPLY) {
      printf("\t\tType: Reply\n");
   } else if (header->type == ICMP_REQUEST) {
      printf("\t\tType: Request\n");
   } else {
      printf("\t\tType: %d\n", header->type);
   }
}