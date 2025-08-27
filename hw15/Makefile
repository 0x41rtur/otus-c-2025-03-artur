# Makefile (OpenGL GLUT)
CC = clang
CFLAGS = -std=c11 -Wall -Wextra -O2

all: gl ascii

gl: main.c
	$(CC) $(CFLAGS) main.c -o main -framework OpenGL -framework GLUT

clean:
	rm -f cube_gl
