NAME := weather
CC := clang
CFLAGS  := -Wall -Wextra -Wpedantic -std=c11

OBJS := main.o
SRCS := main.c external/mjson.c

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(NAME) -lcurl

.PHONY: clean
clean:
	rm -f $(NAME) $(OBJS)