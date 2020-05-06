#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "safeSystemUtil.h"

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

struct Client *getClientFromSocket(struct ClientList *list, int socket)
{
	struct Client *client = list->head;
	while (client != NULL) {
		if (client->socket == socket) {
			break;
		}

		client = client->nextClient;
	}

	return client;
}

void forEachWithPacket(struct ClientList *list, void (*f)(int, char *, uint16_t), char *packet, uint16_t packetSize, int senderSocket)
{
	struct Client *client = list->head;
	while (client != NULL) {
		if (client->handleSet == 1 && client->socket != senderSocket) {
			(*f)(client->socket, packet, packetSize);
		}

		client = client->nextClient;
	}
}

void forEachWithSender(struct ClientList *list, void (*f)(int, char *, uint8_t), int senderSocketNum)
{
	struct Client *client = list->head;
	while (client != NULL) {
		if (client->handleSet == 1) {
			(*f)(senderSocketNum, client->handle, client->handleLength);
		}

		client = client->nextClient;
	}
}

int checkHandleExists(struct ClientList *list, char *handle)
{
	struct Client *client = list->head;
	while (client != NULL) {
		if (client->handle && (strcmp(handle, client->handle) == 0)) {
			return -1;
			break;
		}

		client = client->nextClient;
	}

	return 0;
}

void setClientHandle(struct ClientList *list, int socketNum, char *handle, uint8_t handleSize)
{
	struct Client *client = list->head;
	while (client != NULL) {
		if (client->socket == socketNum) {
			strncpy(client->handle, handle, handleSize);
			(client->handle)[handleSize] = '\0';
			client->handleLength = handleSize;
			client->handleSet = 1;
			break;
		}

		client = client->nextClient;
	}

	// Only increment after handle is set
	list->numClients = list->numClients + 1;
}

void printClient(struct Client *client)
{
	printf("Node:\n\tName: %s\n\tSocket: %d\n", client->handle, client->socket);
}

void *addClient(struct ClientList *list, int socketNum)
{
	struct Client *newClient = (struct Client *) calloc(1, sizeof(struct Client));
	if (newClient == NULL) {
		perror();
		exit(-1);
	}
	newClient->socket = socketNum;

	if (list->head == NULL) {
		list->head = newClient;
		list->numClients = 1;
	}
	else {
		list->tail->nextClient = newClient;
		list->tail = newClient;
	}

	newClient->handleSet = 0;

	return newClient;
}

void removeClientFromList(struct ClientList *list, int socketNum)
{
	struct Client *temp = list->head;
    struct Client *trail = list->head;

	if (temp->socket == socketNum) {
		list->head = temp->nextClient;
		temp = NULL;
	}

	else {
		while (temp != NULL) {
			if (temp->socket == socketNum) {
            trail->nextClient = temp->nextClient;
				break;
			}
         	trail = temp;
			temp = temp->nextClient;
		}
	}

	if (temp == NULL) {
		list->tail = trail;
	} else {
		free(temp);
	}

	list->numClients--;
}