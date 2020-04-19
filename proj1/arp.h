#include <arpa/inet.h>

#define MAC_LENGTH 6

struct arpHeader {
   uint16_t HRD;                       /* Hardware type           - 2 bytes */
	uint16_t PRO;                       /* Protocol Type           - 2 bytes */
	uint8_t HLN;                        /* Hardware Address Length - 1 byte */
	uint8_t PLN;                        /* Protocol Address Length - 1 byte */
	uint16_t OP;                        /* Opcode                  - 2 bytes */
	uint8_t SHA[MAC_LENGTH];            /* Sender Hardware Address */
	uint32_t SPA;                       /* Sender Protocol Address */
	uint8_t THA[MAC_LENGTH];            /* Target Hardware Address */
	uint32_t TPA;                       /* Target Protocol Address */
} __attribute__((packed));