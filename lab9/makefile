TARGET = mysem1 mysem2
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
CFLAGS = -Wall -Wextra -g

.PHONY = all
all: ${TARGET}

clean:
		rm -f $(TARGET) *.o mysem1 mysem2

mysem1: mysem1.c
		gcc -std=c99 $(CFLAGS) $< -o $@ -fsanitize=address

mysem2: mysem2.c
		gcc -std=c99 $(CFLAGS) $< -o $@ -fsanitize=address
	
