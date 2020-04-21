#ifndef ICMP_H
#define ICMP_H

#include <arpa/inet.h>

#define ICMP_REPLY 0x00
#define ICMP_REQUEST 0x08

struct icmpHeader {
	uint8_t type;
} __attribute__((packed));

void icmp(uint8_t *packetData, int packetLength);
void printICMP(struct icmpHeader *header);

#endif