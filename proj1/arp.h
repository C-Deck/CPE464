#include <arpa/inet.h>
#include "globalHelper.h"

struct arpHeader {
   uint16_t HRD;                       /* Hardware type           - 2 bytes ---- UNUSED */
	uint16_t PRO;                       /* Protocol Type           - 2 bytes ---- UNUSED */
	uint8_t HLN;                        /* Hardware Address Length - 1 byte  ---- UNUSED */
	uint8_t PLN;                        /* Protocol Address Length - 1 byte  ---- UNUSED */
	uint16_t OP;                        /* Opcode                  - 2 bytes */
	uint8_t SHA[MAC_LENGTH];            /* Sender Hardware Address - 6 bytes */
	uint32_t SPA;                       /* Sender Protocol Address - 4 bytes */
	uint8_t THA[MAC_LENGTH];            /* Target Hardware Address - 6 bytes */
	uint32_t TPA;                       /* Target Protocol Address - 4 bytes */
} __attribute__((packed));