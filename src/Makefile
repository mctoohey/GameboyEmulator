# File:   Makefile
# Author: Matthew Toohey
# Date:   15/11/2020

# Definitions.
CC = gcc
CFLAGS = -std=c99 -Wall -Wstrict-prototypes -Wextra -g
DEL = rm

EXECUTABLE = gbc
# Windows
ifeq ($(OS),Windows_NT)
	EXECUTABLE = gbc.exe
	DEL = del
	CFLAGS += -mwindows
endif


# Default target.
all: $(EXECUTABLE)


# Compile: create object files from C source files.
main.o: main.c gameboy.h
	$(CC) -c $(CFLAGS) $< -o $@

winmain.o: winmain.c gameboy.h cpu.h
	$(CC) -c $(CFLAGS) $< -o $@

gameboy.o: gameboy.c gameboy.h instructions.h cpu.h memory.h logging.h
	$(CC) -c $(CFLAGS) $< -o $@

cpu.o: cpu.c cpu.h
	$(CC) -c $(CFLAGS) $< -o $@

memory.o: memory.c gameboy.h
	$(CC) -c $(CFLAGS) $< -o $@

screen.o: screen.c screen.h
	$(CC) -c $(CFLAGS) $< -o $@


# Link
$(EXECUTABLE): winmain.o gameboy.o cpu.o memory.o screen.o
	$(CC) $(CFLAGS) $^ -o $@ -lm


# Target: clean project.
.PHONY: clean
clean: 
	-$(DEL) *.o



