CC = gcc
OBJECTS = proxy.o

all: proxy clean

proxy: $(OBJECTS)
	$(CC) -o $@ $^

proxy.o: proxy.c
	$(CC) -c $<

clean:
	rm proxy.o