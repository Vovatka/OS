TARGET = mythreads
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
CFLAGS = -Wall -Wextra -g

.PHONY = all
all: ${TARGET}

clean:
		rm -f $(TARGET) *.o mythreads

mythreads: threads.c
		gcc $(CFLAGS) $< -o $@ -fsanitize=address
	
