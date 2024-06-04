all: client 

clean:
	rm -f client.o client

client: client.o
	cc -g -o client client.o

client.o : client.c
	cc -c -g -Wall -o client.o client.c