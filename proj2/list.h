#ifndef LIST_H
#define LIST_H

#include <stdint.h>

struct Client {
   int socket;
   char handle[101];
   uint8_t handleLength;
   uint8_t handleSet;
   struct Client *nextClient;
};

struct ClientList {
   struct Client *head;
   struct Client *tail;
   int numClients;
};

struct Client *getClient(struct ClientList *list, char *handle);
struct Client *getClientFromSocket(struct ClientList *list, int socket);
void forEachWithPacket(struct ClientList *list, void (*f)(int, char *, uint16_t), char *packet, uint16_t packetSize, int senderSocket);
void forEachWithSender(struct ClientList *list, void (*f)(int, char *, uint8_t), int senderSocketNum);
int checkHandleExists(struct ClientList *list, char *handle);
void setClientHandle(struct ClientList *list, int socketNum, char *handle, uint8_t handleSize);
void printClient(struct Client *client);
void *addClient(struct ClientList *list, int socketNum);
void removeClientFromList(struct ClientList *list, int socketNum);

#endif