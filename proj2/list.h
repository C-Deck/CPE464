#ifndef LIST_H
#define LIST_H

struct Client {
   int socket;
   char handle[101];
   struct Client *nextClient;
};

struct ClientList {
   struct Client *head;
   struct Client *tail;
   int numClients;
};

#endif