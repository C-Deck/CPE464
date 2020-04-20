#include <arpa/inet.h>

struct tcpHeader{
	uint16_t SRC_PORT;                  /* 16 bit field that specifies the port number of the sender */
	uint16_t DEST_PORT;                 /* 16 bit field that specifies the port number of the receiver. */
	uint32_t SEQ_NUM;                   /* 32 bit field that indicates how much data is sent during the TCP session. */
	uint32_t ACK_NUM;                   /* 32 bit field is used by the receiver to request the next TCP segment. This value will be the sequence number incremented by 1. */
	uint8_t NS : 1;                     /* The nonce sum flag is still an experimental flag used to help protect against accidental malicious concealment of packets from the sender */
	uint8_t RSV : 3;                    /* 3 bits for the reserved field. They are unused and are always set to 0 */
	uint8_t DO : 4;                     /* 4 bit data offset (DO) field, also known as the header length. It indicates the length of the TCP header so that we know where the actual data begins */
	uint8_t FIN : 1;                    /* Finish bit is used to end the TCP connection */
	uint8_t SYN : 1;                    /* For the initial three way handshake and it’s used to set the initial sequence number */
	uint8_t RST : 1;                    /* Resets the connection, when you receive this you have to terminate the connection right away */
	uint8_t PSH : 1;                    /* Tells the receiver to process these packets as they are received instead of buffering them */
	uint8_t ACK : 1;                    /* Acknowledge packets which are successful received by the host */
	uint8_t URG : 1;                    /* Used to point to data that is urgently required that needs to reach the receiving process at the earliest */
	uint8_t ECE : 1;                    /* This flag is responsible for indicating if the TCP peer is ECN capable */
	uint8_t CWR : 1;                    /* The congestion window reduced flag is used by the sending host to indicate it received a packet with the ECE flag set. */
	uint16_t WINDOW;                    /* 16 bit window field specifies how many bytes the receiver is willing to receive. */
	uint16_t CHECKSUM;                  /* 16 bits are used for a checksum to check if the TCP header is OK or not */
} __attribute__((packed));