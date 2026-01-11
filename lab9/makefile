TARGET = mysem1 client server
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
CFLAGS = -Wall -Wextra -g

.PHONY = all
all: ${TARGET}

clean:
		rm -f $(TARGET) *.o mysem1 client server

mysem1: mysem1.c
		gcc -std=c99 $(CFLAGS) $< -o $@ -fsanitize=address

server: server.c
		gcc -std=c99 $(CFLAGS) $< -o $@ -fsanitize=address

client: client.c
		gcc -std=c99 $(CFLAGS) $< -o $@ -fsanitize=address
	
