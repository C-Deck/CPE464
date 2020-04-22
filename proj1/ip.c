#include "ip.h"
#include "checksum.h"
#include "udp.h"
#include "icmp.h"
#include "tcp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>


void getIP(const uint8_t *packetData, int packetLength) {
   struct ipHeader *header = (struct ipHeader*) malloc(sizeof(struct ipHeader));
   struct PseudoHeader *pseudo = (struct PseudoHeader*) malloc(sizeof(struct PseudoHeader));
   uint8_t pseudo_header[1500];
   int tcp_size = 0, byteAdjustment = 0;
   uint16_t checksum;

   /* TYPE OF SERVICE */
   header->HDR = *(packetData + byteAdjustment) & 0x0f;
   byteAdjustment++;

   /* TYPE OF SERVICE */
   header->TOS = *(packetData + byteAdjustment);
   byteAdjustment++;

   /* TOTAL LENGTH */
   header->TL = *((uint16_t *) (packetData + byteAdjustment));
   byteAdjustment = byteAdjustment + 2;

   /* IDENTIFICATION */
   header->ID = *((uint16_t *) (packetData + byteAdjustment));
   byteAdjustment = byteAdjustment + 2;

   /* FLAGS */
   header->FLAGS = *((uint16_t *) (packetData + byteAdjustment));
   byteAdjustment = byteAdjustment + 2;

   /* TIME TO LIVE */
   header->TTL = *(packetData + byteAdjustment);
   byteAdjustment++;

   /* PROTOCOL */
   header->PROTOCOL = *(packetData + byteAdjustment);
   byteAdjustment++;

   /* PROTOCOL */
   header->HEADER_CHECKSUM = *((uint16_t *) (packetData + byteAdjustment));
   byteAdjustment = byteAdjustment + 2;

   /* SENDER IP */
   header->SOURCE_ADDR = *((uint32_t *) (packetData + byteAdjustment));
   byteAdjustment = byteAdjustment + IP_LENGTH;

   /* DEST IP */
   header->DEST_ADDR = *((uint32_t *) (packetData + byteAdjustment));
   byteAdjustment = byteAdjustment + IP_LENGTH;

   checksum = in_cksum((uint16_t *) packetData, header->HDR * 4);
   byteAdjustment = header->HDR * 4;
   
   printIP(header, checksum);

   switch (header->PROTOCOL) {
      case ICMP_PROTOCOL:
         getICMP(packetData + byteAdjustment, packetLength - byteAdjustment);
         break;
      case TCP_PROTOCOL:
         tcp_size = pseudoHeader(pseudo_header, header);
         //seudoHeader(pseudo, header);
         getTCP(packetData + byteAdjustment, tcp_size, pseudo_header);
         //getTCP2(packetData + byteAdjustment, pseudo);
         break;
      case UDP_PROTOCOL:
         getUDP(packetData + byteAdjustment, packetLength - byteAdjustment);
         break;
      default:
         break;
   }

   free(header);
   free(pseudo);
}

int pseudoHeader(uint8_t *pseudoHeader, struct ipHeader *header) {
   uint16_t tcp_size = ntohs(header->TL) - (header->HDR * 4);
   uint8_t *ptr = (uint8_t *) &tcp_size;

   memcpy(pseudoHeader, &(header->SOURCE_ADDR), IP_LENGTH);
   memcpy(pseudoHeader + IP_LENGTH, &(header->DEST_ADDR), IP_LENGTH);
   pseudoHeader[8] = 0;
   pseudoHeader[9] = header->PROTOCOL;
   //*((uint16_t *) (pseudoHeader + 10)) = (uint8_t *) &tcp_size;
   //memcpy(pseudoHeader + 10, &tcp_size, 2);
   //pseudoHeader[10] = (uint8_t) *((uint8_t *) ptr);
   //pseudoHeader[11] = *(ptr + 1);
   pseudoHeader[10] = ((tcp_size & 0xff00) >> 8);

   pseudoHeader[11] = tcp_size & 0x00ff;
   printf("\nTCP SIZE %x04\n", tcp_size);
   printf("\n Value wanted -- %x ----- Got: %x  --- and -- %x\n", ((tcp_size & 0xff00) >> 8), *ptr, *(ptr + 1));
   printf("Final value wanted: %x\n", *((uint16_t *)(pseudoHeader + 10)));

   return tcp_size;
}

void seudoHeader(struct PseudoHeader *pseudoHeader, struct ipHeader *header) {
   uint16_t tcp_size = ntohs(header->TL) - (header->HDR * 4);

   pseudoHeader->SOURCE_ADDR = header->SOURCE_ADDR;
   pseudoHeader->DEST_ADDR = header->DEST_ADDR;
   pseudoHeader->ZERO = 0;
   pseudoHeader->PROTOCOL = header->PROTOCOL;
   pseudoHeader->TCP_SIZE = htons(tcp_size);
}

void printIP(struct ipHeader *header, u_int16_t checksum) {
   struct in_addr senderIP = *(struct in_addr *) &(header->SOURCE_ADDR);
   struct in_addr destIP = *(struct in_addr *) &(header->DEST_ADDR);

   printf("\n\tIP Header\n");
	printf("\t\tHeader Len: %d (bytes)\n", header->HDR * WORD_SIZE);
	printf("\t\tTOS: 0x%x\n", header->TOS);
	printf("\t\tTTL: %d\n", header->TTL);
	printf("\t\tIP PDU Len: %d (bytes)\n", ntohs(header->TL));
	printf("\t\tProtocol: %s\n", printProtocol(header));
	printf("\t\tChecksum: %s (0x%x)\n", checksum == 0 ? "Correct" : "Incorrect", header->HEADER_CHECKSUM);
	printf("\t\tSender IP: %s\n", inet_ntoa(senderIP));
	printf("\t\tDest IP: %s\n", inet_ntoa(destIP));
}

char *printProtocol(struct ipHeader *header) {
   switch (header->PROTOCOL) {
      case ICMP_PROTOCOL:
         return "ICMP";
      case TCP_PROTOCOL:
         return "TCP";
      case UDP_PROTOCOL:
         return "UDP";
      default:
         return "Unknown";
   }
}