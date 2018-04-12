CC = gcc
OBJECTS = server.o

all: server clean

server: $(OBJECTS)
	$(CC) -o $@ $^

server.o: server.c
	$(CC) -c $<

clean:
	rm server.o