#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "checksum.h"

#define MAXPDU 88

uint8_t * createPDU(uint32_t sequenceNumber, uint8_t flag, uint8_t *payload, int dataLen)
{
	static uint8_t pduBuffer[MAXPDU];

	// Build the PDU
	((uint32_t *) pduBuffer)[0] = htonl(sequenceNumber);
	pduBuffer[6] = flag;
	memcpy(&pduBuffer[7], payload, dataLen);
    
    // Do checksum on pdu after payload has been copied
	((uint16_t *) pduBuffer)[2] = in_cksum((unsigned short *)pduBuffer, dataLen + 7);

	return pduBuffer;
}

void outputPDU(uint8_t * aPDU, int pduLength)
{
    uint16_t checksum = 0;
    uint8_t flag = 0;
    uint32_t sequenceNumber = 0;

    sequenceNumber = ntohl(((uint32_t *) aPDU)[0]);
    checksum = ((uint16_t *) aPDU)[2];
    flag = aPDU[6];

    printf("Sequence Number: %d\n", sequenceNumber);
    printf("Checksum: %d\n", checksum);
    printf("Flag: %d\n", flag);
    printf("Payload: %.*s\n", pduLength - 7, aPDU + 7);
}