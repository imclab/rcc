# Make file by Dylan Swiggett
# Written on Linux Mint 11
# Probably doesn't work on windows, haven't tested...
# 
# make objects - compile all c files to objects
# make exe - compile all objects to an executable
# make clean - remove all object files
# make all - compile objects, make executable, clean directory
# make run - do full compile and clean, then run

CFLAGS = -Wall -lm
FILES = main.c rtree.c
OBJECTS = main.o rtree.o
MAIN = main
CC = gcc
OUTPUT = rtree

objects:
	$(CC) -c $(FILES)

exe:
	$(CC) $(CFLAGS) $(OBJECTS) -o $(OUTPUT)

clean:
	rm $(OBJECTS)

all:
	make objects
	make exe
	make clean

run:
	make all
	./$(OUTPUT)