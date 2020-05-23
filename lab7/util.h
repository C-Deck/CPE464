#include <sys/types.h>

#define MAX_BUFFER 1500

void outputPDU(uint8_t * aPDU, int pduLength);
uint8_t * createPDU(uint32_t sequenceNumber, uint8_t flag, uint8_t *payload, int dataLen);