#ifndef TCP_H
#define TCP_H

#include <arpa/inet.h>

#define ACK_MASK 0x10
#define SYN_MASK 0x02
#define RST_MASK 0x04
#define FIN_MASK 0x01

#define HTTP 80

struct tcpHeader {
	uint16_t SOURCE_PORT;               /* 16 bit field that specifies the port number of the sender */
	uint16_t DEST_PORT;                 /* 16 bit field that specifies the port number of the receiver. */
	uint32_t SEQ_NUM;                   /* 32 bit field that indicates how much data is sent during the TCP session. */
	uint32_t ACK_NUM;                   /* 32 bit field is used by the receiver to request the next TCP segment. This value will be the sequence number incremented by 1. */
	uint8_t FLAGS;						/* 8 bit field used for the flags */
	uint16_t WINDOW;                    /* 16 bit window field specifies how many bytes the receiver is willing to receive. */
	uint16_t CHECKSUM;                  /* 16 bits are used for a checksum to check if the TCP header is OK or not */
} __attribute__((packed));

void printTCP(struct tcpHeader *header, uint16_t checksum);
void getTCP(const uint8_t *packetData, int tcp_size, uint8_t *psuedoHeader);

#endif