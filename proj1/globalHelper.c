#include <arpa/inet.h>

void printMAC(uint8_t *mac) {
   printf("%x:%x:%x:%x:%x:%x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}