#include <arpa/inet.h>

struct tcpHeader{
	uint16_t src_port;
	uint16_t dest_port;
	uint32_t seq;
	uint32_t ack_num;
	uint8_t ns : 1;
	uint8_t reserved : 3;
	uint8_t offset : 4;
	uint8_t fin : 1;
	uint8_t syn : 1;
	uint8_t rst : 1;
	uint8_t psh : 1;
	uint8_t ack : 1;
	uint8_t urg : 1;
	uint8_t ece : 1;
	uint8_t cwr : 1;
	uint16_t window_size;
	uint16_t checksum;
} __attribute__((packed));