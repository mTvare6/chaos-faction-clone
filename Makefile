all: src/main.c
	gcc -Iincludes -framework IOKit -framework Cocoa -framework OpenGL $(shell pkg-config --libs --cflags raylib) src/main.c -o main -Wextra -Wall -Wunused -pedantic
