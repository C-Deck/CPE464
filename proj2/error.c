#include <stdio.h>

void handleInUse(char *handle) {
    printf("Handle already in use: %s\n", handle);
}

void handleTooLong(char *handle) {
    printf("Invalid handle, handle longer than 100 characters: %s\n", handle);
}

void badHandle() {
    printf("Invalid handle, handle starts with a number\n");
}

void serverTerminated() {
    printf("Server Terminated\n");
}

void handleNotFound(char *handle) {
    printf("Client with handle %s does not exist.\n", handle);
}

void invalidCommand() {
    printf("Invalid Command\n");
}

void invalidFormat() {
    printf("Invalid command format\n");
}

void tooManyHandles() {
    printf("Messages can only be send to a maximum of 9 handles\n");
}