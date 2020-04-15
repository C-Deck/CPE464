#include <stdio.h>
#include <string.h>
#include <pcap.h>

int main(int argc, char **argv) {
		char errbuf[PCAP_ERRBUF_SIZE];   /* error buffer */
      pcap_t *pcap;                    /* packet capture descriptor */
      char fname[80];                  /* name of savefile to read packet data from */
      const u_char *packet;		      /* The actual packet */
      struct pcap_pkthdr *pkt_header;  /* Packet header */
      int nextResult;

      if (argc != 2) {
         return -1;                    /* Bad call */
      }

      strcpy(fname, argv[2]);          /* Get the filename to use */

      if (!(pcap = pcap_open_offline(fname, errbuf))) {
         return -1;                    /* Could not open the file */
      }

      while ((nextResult = pcap_next_ex(pcap, &pkt_header, &packet)) >= 0) {
         if (nextResult == 0) {
            continue;
         }
      }

      if (nextResult == PCAP_ERROR) {  /* Check for error instead of finish */
         printf("Error reading the packets: %s\n", pcap_geterr(pcap));
         return -1;
      }

      pcap_close(pcap);                /* Close the descriptor */
		return 0;
}