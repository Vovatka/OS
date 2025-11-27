TARGET = mypipe
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
CFLAGS = -Wall -Wextra -g

.PHONY = all
all: ${TARGET}

clean:
		rm -f $(TARGET) *.o mypipe

mypipe: pipef.c
		gcc $(CFLAGS) $< -o $@ -fsanitize=address
	
