#include <arp.h>
const char *arp_opcodes[10] = {"Unknown", "Request", "Reply", "Rev Request", "Rev Reply", "Unknown", "Unknown", "Unknown", "INV Request", "INV Reply"};

void getARP(uint8_t *packetData, int packetLength) {
   //TODO
}

void printARP(struct arpHeader *header) {
   printf("\n\tARP header\n");
	printf("\t\tOpcode: %s\n", arp_opcodes[header->OP]);
	printf("\t\tSender MAC: ");
   printMAC(header->SHA);
	printf("\t\tSender IP: %s\n", header->SPA);
	printf("\t\tTarget MAC: ");
   printMAC(header->THA);
	printf("\t\tTarget IP: %s\n", header->TPA);
}