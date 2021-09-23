all: src/main.c
	gcc -Iincludes -Iraygui -framework IOKit -framework Cocoa -framework OpenGL $(shell pkg-config --libs --cflags raylib) src/main.c -o main -Wextra -Wall -Wunused -pedantic

raygui:
	mkdir raygui
	wget "https://raw.githubusercontent.com/raysan5/raygui/master/src/raygui.h" -O raygui/raygui.h
