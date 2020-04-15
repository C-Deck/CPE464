#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <pcap.h>
#include <arpa/inet.h>

#define MAC_LENGTH 6

#define ARP_TYPE 1
#define IP_TYPE 2

#define ARP_TYPE_HEX 0x0806
#define IP_TYPE_HEX 0x0800

/*  struct pcap_pkthdr { 
   struct timeval ts;   time stamp          
   bpf_u_int32 caplen;  length of portion present
   bpf_u_int32;         length this packet
} */

struct ethernetHeader {
   u_char dest[MAC_LENGTH];            /* Destination MAC   - 4 bytes */
	u_char source[MAC_LENGTH];          /* Source MAC        - 4 bytes */
	u_short type;                       /* Type              - 2 bytes */
};

void printEthernet(struct ethernetHeader *ethernet);
void printType(u_short typeHex);
void printMAC(u_char *mac);
int getType(u_short type);

int main(int argc, char **argv) {
		char errbuf[PCAP_ERRBUF_SIZE];   /* error buffer */
      pcap_t *pcap;                    /* packet capture descriptor */
      char fname[80];                  /* name of savefile to read packet data from */
      const u_char *packet;		      /* The actual packet */
      struct pcap_pkthdr *pkt_header;  /* Packet header */
      struct ethernetHeader *ethernet = (struct ethernetHeader*) malloc(sizeof(struct ethernetHeader)); /* The ethernet header */
      int nextResult, packetCount = 1;

      if (argc != 2) {
         return -1;                    /* Bad call */
      }

      strcpy(fname, argv[1]);          /* Get the filename to use */

      if (!(pcap = pcap_open_offline(fname, errbuf))) {
         return -1;                    /* Could not open the file */
      }

      while ((nextResult = pcap_next_ex(pcap, &pkt_header, &packet)) >= 0) {
         if (nextResult == 0) {        /* Timeout check */
            continue;
         }
         memcpy(ethernet->dest, packet, MAC_LENGTH);
         memcpy(ethernet->source, packet + MAC_LENGTH, MAC_LENGTH);
         ethernet->type = *((u_short *) (packet + MAC_LENGTH * 2));
         printf("\nPacket number: %d  Frame Len: %d\n", packetCount++, pkt_header->len);

         printEthernet(ethernet);
      }

      if (nextResult == PCAP_ERROR) {  /* Check for error instead of finish */
         printf("Error reading the packets: %s\n", pcap_geterr(pcap));
         return -1;
      }

      free(ethernet);                  /* Free dynamic memory */
      pcap_close(pcap);                /* Close the descriptor */
		return 0;
}

void printEthernet(struct ethernetHeader *ethernet) {
   printf("\n\tEthernet Header\n");
   printf("\t\tDest MAC: ");
   printMAC(ethernet->dest);
   printf("\t\tSource MAC: ");
   printMAC(ethernet->source);
   printf("\t\tType: ");
   printType(ethernet->type);
}

void printType(u_short typeHex) {
   int type = getType(typeHex);
   printf("%s", type == ARP_TYPE ? "ARP\n" : "IP\n");
}

int getType(u_short type) {
   if (ntohs(type) == ARP_TYPE_HEX) {
      return ARP_TYPE;
   } else if (ntohs(type) == IP_TYPE_HEX) {
      return IP_TYPE;
   } else {
      return -1;
   }
}

void printMAC(u_char *mac) {
   printf("%x:%x:%x:%x:%x:%x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
