#ifndef UDP_H
#define UDP_H

#include <arpa/inet.h>

struct udpHeader{
	uint16_t SOURCE_PORT;
	uint16_t DEST_PORT;
} __attribute__((packed));

#endif