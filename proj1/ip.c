#include "ip.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void ip(uint8_t *packetData, int packetLength) {
   struct ipHeader *header = (struct ipHeader*) malloc(sizeof(struct ipHeader));
   uint8_t pseudo_header[1500];
   int tcp_size = 0;
   
   printIP(header);

   switch (header->PROTOCOL) {
      case ICMP_PROTOCOL:
         //
         break;
      case TCP_PROTOCOL:
         //
         break;
      case UDP_PROTOCOL:
         //
         break;
      default:
         break;
   }

   free(header);
}

void printIP(struct ipHeader *header) {
   struct in_addr senderIP = *(struct in_addr *) &(header->SOURCE_ADDR);
   struct in_addr destIP = *(struct in_addr *) &(header->DEST_ADDR);

   printf("\n\tIP Header\n");
	printf("\t\tHeader Len: %d (bytes)\n", header->TL * 4);
	printf("\t\tTOS: %x\n", header->TOS);
	printf("\t\tTTL: %d\n", header->TTL);
	printf("\t\tIP PDU Len: %d (bytes)\n", pduLength);
	printf("\t\tProtocol: %s\n", printProtocol(header));
	printf("\t\tChecksum: %s (%x04)\n", goodCheck ? "Correct" : "Incorrect", header->HEADER_CHECKSUM);
	printf("\t\tSender IP: %s\n", inet_ntoa(senderIP));
	printf("\t\tDest IP: %s\n", inet_ntoa(destIP));
}

char *printProtocol(struct ipHeader *header){
   switch (header->PROTOCOL) {
      case ICMP_PROTOCOL:
         return "ICMP";
         break;
      case TCP_PROTOCOL:
         return "TCP";
      case UDP_PROTOCOL:
         return "UDP";
      default:
         return "Unknown";
   }
}