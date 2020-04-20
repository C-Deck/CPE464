#include <arpa/inet.h>

#define ICMP_PROTOCOL 1
#define TCP_PROTOCOL 6
#define UDP_PROTOCOL 17

struct ipHeader {
	uint8_t HDR : 4;
	uint8_t VERSION : 4;
	uint8_t TOS;
	uint16_t TL;
	uint16_t ID;
	uint16_t FLAGS;
	uint8_t TTL;
	uint8_t PROTOCOL;
	uint16_t HEADER_CHECKSUM;
	uint32_t SOURCE_ADDR;
	uint32_t DEST_ADDR;
} __attribute__((packed));