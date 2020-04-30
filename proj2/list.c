#include "list.h"
#include "safeSystemUtil.h"
#include <string.h>

struct Client *getClient(struct ClientList *list, char *handle)
{
	struct Client *client = list->head;
	while (client != NULL) {
		if (client->handle && (strcmp(handle, client->handle) == 0)) {
			break;
		}

		client = client->nextClient;
	}

	return client;
}

void printClient(struct Client *client)
{
	printf("Node:\n\tName: %s\n\tSocket: %d\n", client->handle, client->socket);
}

struct Client *addClient(struct ClientList *list)
{
	struct Client *newClient = safeCalloc(1, sizeof(struct Client));

	if (list->head == NULL) {
		list->head = newClient;
		list->numClients = 1;
	}
	else {
		list->tail->nextClient = newClient;
		list->tail = newClient;
      list->numClients = list->numClients + 1;
	}

	return newClient;
}

void removeClient(struct ClientList *list, struct Client *client)
{
	struct Client *temp = list->head;
   struct Client *trail = list->head;

	if (temp == client) {
		list->head = client->nextClient;
		temp = NULL;
	}

	else {
		while (temp != NULL) {
			if (temp == client) {
            trail->nextClient = client->nextClient;
				break;
			}
         trail = temp;
			temp = temp->nextClient;
		}
	}

	if (temp == NULL) {
		list->tail = trail;
	}

	close(client->socket);
	free(client);
}