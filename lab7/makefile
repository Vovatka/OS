CXX = gcc
CXXFLAGS = -Wall -Wextra -std=c11

all: myreader mywriter

myreader: reader.c
	$(CC) $(CFLAGS) -o myreader reader.c

mywriter: writer.c
	$(CC) $(CFLAGS) -o mywriter writer.c

clean:
	rm -f myreader mywriter

