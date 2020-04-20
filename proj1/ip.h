#include <arpa/inet.h>

struct ipHeader {
	uint8_t HDR : 4;
	uint8_t VERSION : 4;
	uint8_t ECN : 2;
	uint8_t DSC : 6;
	uint16_t TL;
	uint16_t ID;
	uint16_t FLAGS;
	uint8_t TTL;
	uint8_t PROTOCOL;
	uint16_t HEADER_CHECKSUM;
	uint32_t SOURCE_ADDR;
	uint32_t DEST_ADDR;
} __attribute__((packed));