#ifndef ERROR_H
#define ERROR_H

void handleInUse(char *handle);
void handleTooLong(char *handle);
void badHandle();
void serverTerminated();
void handleNotFound(char *handle);
void invalidCommand();
void invalidFormat();
void tooManyHandles();

#endif