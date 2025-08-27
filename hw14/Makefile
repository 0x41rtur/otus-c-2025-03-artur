CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
LIBS = -lsqlite3

TARGET = main
SRC = main.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET) *.o
