all: client server

client: client.c protocol.h
	gcc -o client client.c -Wall

server: server.c protocol.h
	gcc -o server server.c -Wall -lpthread
