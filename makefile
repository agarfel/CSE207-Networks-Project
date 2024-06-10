all: client server

clean:
	rm -f client.o client server server.o

client: client.o
	cc -g -o client client.o

client.o : client.c
	cc -c -g -Wall -o client.o client.c

myserver: myserver.o
	cc -g -o server server.o

myserver.o : myserver.c
	cc -c -g -Wall -o server.o server.c