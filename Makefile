CC = g++
CFLAGS = -Wall -Wextra -Wpedantic

all: Client Server

Server:
	$(CC) Server.cpp -o Server $(CFLAGS)

Client:
	$(CC) Client.cpp -o Client $(CFLAGS)

clean:
	rm -f Server Client
