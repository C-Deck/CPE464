#include <safeSystemUtil.h>

int safeSocket() {
    int server_fd = 0;

    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    }

    return server_fd;
}

void safeConnect(int sockfd, struct sockaddr *serv_addr, int addrlen) {

    if (connect(sockfd, &serv_addr, addrlen) < 0)
    { 
        perror("connect call");
		exit(-1);
    } 
}

int safeAccept(int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen) {
    int client_socket = 0;

    if ((client_socket = accept(sockfd, cliaddr, addrlen)) < 0)
	{
		perror("accept call error");
		exit(-1);
	}

    return(client_socket);
}

void safeBind(int sockfd, struct sockaddr *my_addr, int addrlen) {
    if (bind(sockfd, my_addr, addrlen) < 0)
	{
		perror("bind call");
		exit(-1);
	}
}

void safeListen(int sockfd, int backlog) {
    if (listen(sockfd, backlog) < 0)
	{
		perror("listen call");
		exit(-1);
	}
}

int safeRecv(int sockfd, void *buf, int len, unsigned int flags) {
    int messageLen = 0;
    
    //now get the data from the client_socket (message includes null)
	if ((messageLen = recv(sockfd, buf, len, flags)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

    return messageLen;
}

int safeSend(int sockfd, const void *msg, int len, int flags) {
	int sentSize = 0;

	if ((sentSize = send(sockfd, msg, len, flags)) < 0)
	{
		perror("send call");
		exit(-1);
	}

	return sentSize;
}

void safeClose(int sockfd) {
    if (close(sockfd) < 0) {
        perror("Close call");
        exit(-1);
    }
}

void * safeRealloc(void *ptr, size_t size)
{
	void * returnValue = NULL;
	
	if ((returnValue = realloc(ptr, size)) == NULL)
	{
		printf("Error on realloc (tried for size: %d\n", (int) size);
		exit(-1);
	}
	
	return returnValue;
} 

void * safeCalloc(size_t nmemb, size_t size)
{
	void * returnValue = NULL;
	if ((returnValue = calloc(nmemb, size)) == NULL)
	{
		perror("calloc");
		exit(-1);
	}
	return returnValue;
}

void * safeMalloc(size_t size)
{
	void * returnValue = NULL;
	if ((returnValue = malloc(size)) == NULL)
	{
		perror("malloc");
		exit(-1);
	}
	return returnValue;
}