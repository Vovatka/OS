TARGET = mycond
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
CFLAGS = -Wall -Wextra -g

.PHONY = all
all: ${TARGET}

clean:
		rm -f $(TARGET) *.o mycond

mycond: mycond.c
		gcc -std=c99 $(CFLAGS) $< -o $@ -fsanitize=address
	
