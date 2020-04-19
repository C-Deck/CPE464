#include <arpa/inet.h>

struct udpHeader{
	uint16_t src;
	uint16_t dest;
} __attribute__((packed));