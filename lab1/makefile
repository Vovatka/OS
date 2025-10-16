TARGET = mycat mygrep
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
CFLAGS = -Wall -Wextra -g

.PHONY = all memcheck-cat memcheck-grep
all: ${TARGET}

clean:
		rm -f $(TARGET) *.o mycat mygrep

mycat: cat.c
		gcc -std=c99 $(CFLAGS) $< -o $@ -fsanitize=address

mygrep: grep.c
		gcc -std=c99 $(CFLAGS) $< -o $@ -fsanitize=address
	
