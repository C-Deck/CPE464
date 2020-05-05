#ifndef SAFE_CALLS_H
#define SAFE_CALLS_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

int safeSocket();
void safeConnect(int sockfd, struct sockaddr *serv_addr, int addrlen);
int safeAccept(int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen);
void safeBind(int sockfd, struct sockaddr *my_addr, int addrlen);
void safeListen(int sockfd, int backlog);
int safeRecv(int sockfd, void *buf, int len, unsigned int flags);
int safeSend(int sockfd, const void *msg, int len, int flags);
void safeClose(int sockfd);
void * safeRealloc(void *ptr, size_t size);
void * safeCalloc(size_t nmemb, size_t size);
void * safeMalloc(size_t size);

#endif