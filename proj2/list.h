#ifndef LIST_H
#define LIST_H

struct Client {
   int socket;
   char handle[101];
   uint8_t handleLength;
   struct Client *nextClient;
};

struct ClientList {
   struct Client *head;
   struct Client *tail;
   int numClients;
};

struct Client *getClient(struct ClientList *list, char *handle);
void doOnEachClient(struct ClientList *list, void (*f)(int, char *, uint8_t), int senderSocketNum);
int checkHandleExists(struct ClientList *list, char *handle);
void setClientHandle(struct ClientList *list, int socketNum, char *handle, int handleSize);
void printClient(struct Client *client);
void *addClient(struct ClientList *list, int socketNum);
void removeClient(struct ClientList *list, struct Client *client);

#endif