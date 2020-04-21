#include "arp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void getARP(const uint8_t *packetData, int packetLength) {
   /* START ADJUSTMENT BY 8 BYTES FOR HDR, PRO, HLN, PLN */
   int byteAdjustment = 8;
   struct arpHeader *header = (struct arpHeader*) malloc(sizeof(struct arpHeader));

   /* OPCODE */
   header->OP = ntohs(*((uint16_t *) (packetData + byteAdjustment)));
   byteAdjustment = byteAdjustment + 2;

   /* SENDER MAC */
   memcpy(header->SHA, packetData + byteAdjustment, MAC_LENGTH);
   byteAdjustment = byteAdjustment + MAC_LENGTH;

   /* SENDER IP */
   header->SPA = *((uint32_t *) (packetData + byteAdjustment));
   byteAdjustment = byteAdjustment + IP_LENGTH;

   /* TARGET MAC */ 
   memcpy(header->THA, packetData + byteAdjustment, MAC_LENGTH);
   byteAdjustment = byteAdjustment + MAC_LENGTH;

   /* TARGET IP */
   header->TPA = *((uint32_t *) (packetData + byteAdjustment));
   byteAdjustment = byteAdjustment + IP_LENGTH;

   printARP(header);
   free(header);
}

void printARP(struct arpHeader *header) {
   struct in_addr senderIP = *(struct in_addr *) &(header->SPA);
   struct in_addr targetIP = *(struct in_addr *) &(header->TPA);

   printf("\n\tARP header\n");
	printf("\t\tOpcode: %s\n", getARPCode(header->OP));
	printf("\t\tSender MAC: ");
   printMAC(header->SHA);
	printf("\t\tSender IP: %s\n", inet_ntoa(senderIP));
	printf("\t\tTarget MAC: ");
   printMAC(header->THA);
	printf("\t\tTarget IP: %s\n", inet_ntoa(targetIP));
}

void printMAC(uint8_t *mac) {
   printf("%x:%x:%x:%x:%x:%x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

char *getARPCode(uint16_t op) {
   switch (op) {
      case ARP_REQUEST_OP:
         return ARP_REQUEST;
      case ARP_REPLY_OP:
         return ARP_REPLY;
      case ARP_REV_REQUEST_OP:
         return ARP_REV_REQUEST;
      case ARP_REV_REPLY_OP:
         return ARP_REV_REPLY;
      case ARP_INV_REQUEST_OP:
         return ARP_INV_REQUEST;
      case ARP_INV_REPLY_OP:
         return ARP_INV_REPLY;
      default:
         return "Unknown";
   }
}
