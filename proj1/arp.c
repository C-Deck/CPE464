#include "arp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const char *arp_opcodes[10] = {"Unknown", "Request", "Reply", "Rev Request", "Rev Reply", "Unknown", "Unknown", "Unknown", "INV Request", "INV Reply"};

void getARP(const uint8_t *packetData, int packetLength) {
   /* START ADJUSTMENT BY 8 BYTES FOR HDR, PRO, HLN, PLN */
   int byteAdjustment = 8;
   struct arpHeader *header = (struct arpHeader*) malloc(sizeof(struct arpHeader));

   /* OPCODE */
   header->OP = *((uint16_t *) (packetData + byteAdjustment));
   byteAdjustment = byteAdjustment + 2;

   /* SENDER MAC */
   memcpy(header->SHA, packetData, MAC_LENGTH);
   byteAdjustment = byteAdjustment + MAC_LENGTH;

   /* SENDER IP */
   header->SPA = *((uint32_t *) (packetData + byteAdjustment));
   byteAdjustment = byteAdjustment + IP_LENGTH;

   /* TARGET MAC */ 
   memcpy(header->THA, packetData, MAC_LENGTH);
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
	printf("\t\tOpcode: %s\n", arp_opcodes[((int)header->OP)]);
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
