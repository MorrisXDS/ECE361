CC=gcc
CFLAGS=-I./server -I./message -I./client
DEBUG=-g
THD=-pthread

all: server client
server: server/server.o message/message.o server/user.o
	$(CC) $(CFLAGS) $(THD) ${DEBUG} server/server.o message/message.o server/user.o -o server/server
client: client/client.o message/message.o
	$(CC) $(CFLAGS) $(THD) ${DEBUG} client/client.o message/message.o -o client/client
server.o: server/server.c  server/server.h message/message.h server/user.h
	$(CC) $(CFLAGS) $(THD) ${DEBUG} server/server.c -c server.o
client.o: client/client.c  client/client.h message/message.h
	$(CC) $(CFLAGS) $(THD) ${DEBUG} client/client.c -c client.o
message.o: message/message.c message/message.h
	$(CC) $(CFLAGS) $(THD) ${DEBUG} message/message.c -c message.o
user.o: server/user.c server/user.h server/server.h message/message.h
	$(CC) $(CFLAGS) $(THD) ${DEBUG} server/user.c -c user.o
clean:
	rm -rf */*.o */server */client