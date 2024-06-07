all: client myserver

clean:
	rm -f client.o client

client: client.o
	cc -g -o client client.o

client.o : client.c
	cc -c -g -Wall -o client.o client.c

myserver: myserver.o
	cc -g -o myserver myserver.o

myserver.o : myserver.c
	cc -c -g -Wall -o myserver.o myserver.c