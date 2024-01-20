CC=gcc
CFLAGS=-I./server -I./packets -I./client
THD=-pthread

all: server client
server: server/server.o packets/packets.o
	$(CC) $(CFLAGS) $(THD) server/server.o packets/packets.o -o server/server 
client: client/client.o packets/packets.o
	$(CC) $(CFLAGS) $(THD) client/client.o packets/packets.o -o client/client
server.o: server/server.c  server/server.h packets/packets.h
	$(CC) $(CFLAGS) $(THD) server/server.c -c server.o
client.o: client/client.c  client/client.h packets/packets.h
	$(CC) $(CFLAGS) $(THD) client/client.c -c client.o
packets.o: packets/packets.c packets/packets.h
	$(CC) $(CFLAGS) $(THD) packets/packets.c -c packets.o
clean:
	rm -rf */*.o */server */client