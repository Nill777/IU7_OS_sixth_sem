CC = gcc
CFLAGS = -Wall -g
OBJ = client.o server.o
HEADER = calc.h

all: client server

client: client.o
	$(CC) -o client client.o

server: server.o
	$(CC) -o server server.o

client.o: client.c $(HEADER)
	$(CC) $(CFLAGS) -c client.c

server.o: server.c $(HEADER)
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -f $(OBJ) client server
