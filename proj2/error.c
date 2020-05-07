#include <stdio.h>

void handleInUse(char *handle) {
    printf("\nHandle already in use: %s\n", handle);
}

void handleTooLong(char *handle) {
    printf("\nInvalid handle, handle longer than 100 characters: %s\n", handle);
}

void badHandle() {
    printf("\nInvalid handle, handle starts with a number\n");
}

void serverTerminated() {
    printf("\nServer Terminated\n");
}

void handleNotFound(char *handle) {
    printf("\nClient with handle %s does not exist.\n", handle);
}

void invalidCommand() {
    printf("\nInvalid Command\n");
}

void invalidFormat() {
    printf("\nInvalid command format\n");
}

void tooManyHandles() {
    printf("\nMessages can only be send to a maximum of 9 handles\n");
}