# Makefile for CPE464 tcp test code
# written by Hugh Smith - April 2019

CC= gcc
CFLAGS= -Wall
LIBS = 


all:   cclient server

cclient: myClient.c list.c pollLib.c error.c safeSystemUtil.c networks.o gethostbyname6.o *.h
	$(CC) $(CFLAGS) -o cclient myClient.c list.c pollLib.c error.c safeSystemUtil.c networks.o gethostbyname6.o $(LIBS)

server: myServer.c list.c pollLib.c error.c safeSystemUtil.c networks.o gethostbyname6.o *.h
	$(CC) $(CFLAGS) -o server myServer.c list.c pollLib.c error.c safeSystemUtil.c networks.o gethostbyname6.o $(LIBS)

.c.o:
	gcc -c $(CFLAGS) $< -o $@ $(LIBS)

cleano:
	rm -f *.o

clean:
	rm -f server cclient *.o




