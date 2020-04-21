#ifndef ARP_H
#define ARP_H

#include <arpa/inet.h>
#define MAC_LENGTH 6
#define IP_LENGTH 4

#define ARP_REQUEST_OP 1
#define ARP_REPLY_OP 2
#define ARP_REV_REQUEST_OP 3
#define ARP_REV_REPLY_OP 4
#define ARP_INV_REQUEST_OP 8
#define ARP_INV_REPLY_OP 9

#define ARP_REQUEST "Request"
#define ARP_REPLY "Reply"
#define ARP_REV_REQUEST "Rev Request"
#define ARP_REV_REPLY "Rev Reply"
#define ARP_INV_REQUEST "INV Request"
#define ARP_INV_REPLY "INV Reply"

struct arpHeader {
   uint16_t HRD;                        /* Hardware type           - 2 bytes ---- UNUSED */
	uint16_t PRO;                       /* Protocol Type           - 2 bytes ---- UNUSED */
	uint8_t HLN;                        /* Hardware Address Length - 1 byte  ---- UNUSED */
	uint8_t PLN;                        /* Protocol Address Length - 1 byte  ---- UNUSED */
	uint16_t OP;                        /* Opcode                  - 2 bytes */
	uint8_t SHA[MAC_LENGTH];            /* Sender Hardware Address - 6 bytes */
	uint32_t SPA;                       /* Sender Protocol Address - 4 bytes */
	uint8_t THA[MAC_LENGTH];            /* Target Hardware Address - 6 bytes */
	uint32_t TPA;                       /* Target Protocol Address - 4 bytes */
} __attribute__((packed));

void printARP(struct arpHeader *header);
void getARP(const uint8_t *packetData, int packetLength);
void printMAC(uint8_t *mac);
char *getARPCode(uint16_t op);

#endif