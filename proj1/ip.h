#include <arpa/inet.h>

struct ipHeader {
	uint8_t hdr_len : 4;
	uint8_t version : 4;
	uint8_t ecn : 2;
	uint8_t dsc : 6;
	uint16_t tot_len;
	uint16_t id;
	uint16_t flags_frag_offset;
	uint8_t ttl;
	uint8_t proto;
	uint16_t checksum;
	uint32_t src;
	uint32_t dest;
} __attribute__((packed));