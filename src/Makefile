CC=gcc
CFLAGS=-c `sdl-config --cflags` -Wall
LIBS=`sdl-config --libs` -lSDL_image -lSDL_ttf -lpng

all: nhtexttile

nhtexttile: main.o
	$(CC) $(LIBS) -o $@ $+

main.o: main.c
	$(CC) $(CFLAGS) -o $@ $+

.PHONY: clean

clean:
	rm -f *.o nhtexttile
