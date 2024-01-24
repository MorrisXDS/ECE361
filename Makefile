CC=gcc
CFLAGS=-I./server -I./message -I./client
THD=-pthread

all: server client
server: server/server.o message/message.o server/session.o server/user.o
	$(CC) $(CFLAGS) $(THD) server/server.o message/message.o server/session.o server/user.o -o server/server
client: client/client.o message/message.o
	$(CC) $(CFLAGS) $(THD) client/client.o message/message.o -o client/client
server.o: server/server.c  server/server.h message/message.h server/session.h server/user.h
	$(CC) $(CFLAGS) $(THD) server/server.c -c server.o
client.o: client/client.c  client/client.h message/message.h
	$(CC) $(CFLAGS) $(THD) client/client.c -c client.o
message.o: message/message.c message/message.h
	$(CC) $(CFLAGS) $(THD) message/message.c -c message.o
session.o: server/session.c server/session.h server/server.h message/message.h
	$(CC) $(CFLAGS) $(THD) server/session.c -c session.o
user.o: server/user.c server/user.h message/message.h
	$(CC) $(CFLAGS) $(THD) server/user.c -c user.o
clean:
	rm -rf */*.o */server */client