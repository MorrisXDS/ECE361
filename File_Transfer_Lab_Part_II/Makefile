CC=gcc
CFLAGS=-I./packets

all: server deliver

server: server/server.o packets/packet.o
	$(CC) $(CFLAGS) -o server/server server/server.o packets/packet.o

deliver: deliver/deliver.o packets/packet.o
	$(CC) $(CFLAGS) -o deliver/deliver deliver/deliver.o packets/packet.o

server/server.o: server/server.c packets/packet.h
	$(CC) $(CFLAGS) -c server/server.c -o server/server.o

deliver/deliver.o: deliver/deliver.c packets/packet.h
	$(CC) $(CFLAGS) -c deliver/deliver.c -o deliver/deliver.o

packets/packet.o: packets/packet.c packets/packet.h
	$(CC) $(CFLAGS) -c packets/packet.c -o packets/packet.o

clean:
	rm -f server/*.o deliver/*.o packets/*.o server/server deliver/deliver
