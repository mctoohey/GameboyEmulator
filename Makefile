# File:   Makefile
# Author: Matthew Toohey
# Date:   15/11/2020

# Definitions.
CC = gcc
CFLAGS = -std=c99 -Wall -Wstrict-prototypes -Wextra -g -Isrc/common
CLEAN_CMD = rm build/obj/* && rm build/bin/*

SRC_DIR = src
COMMON_DIR = $(SRC_DIR)/common
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

MAIN = main
MAIN_DIR = $(SRC_DIR)
MAIN_DEPS = $(COMMON_DIR)/gameboy.h $(COMMON_DIR)/cpu.h
EXECUTABLE = gbc

COPY_BTLDR_CMD = cp DMG_ROM.bin build/bin
# Windows
ifeq ($(OS),Windows_NT)
	EXECUTABLE = gbc.exe
	MAIN = winmain
	MAIN_DIR = $(SRC_DIR)/windows
	MAIN_DEPS = $(COMMON_DIR)/gameboy.h $(COMMON_DIR)/cpu.h
	CFLAGS += -mwindows
	CLEAN_CMD = del /Q build\obj\* && del /Q build\bin\*
	COPY_BTLDR_CMD = copy DMG_ROM.bin build\bin 
endif


# Default target.
all: $(BIN_DIR)/$(EXECUTABLE) $(BIN_DIR)/DMG_ROM.bin


# Compile: create object files from C source files.
$(OBJ_DIR)/$(MAIN).o: $(MAIN_DIR)/$(MAIN).c $(MAIN_DEPS)
	$(CC) -c $(CFLAGS) $< -o $@

# winmain.o: winmain.c gameboy.h cpu.h
# 	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR)/gameboy.o: $(COMMON_DIR)/gameboy.c $(COMMON_DIR)/gameboy.h $(COMMON_DIR)/cpu.h $(COMMON_DIR)/instructions.h $(COMMON_DIR)/logging.h $(COMMON_DIR)/memory.h $(COMMON_DIR)/screen.h
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR)/cpu.o: $(COMMON_DIR)/cpu.c $(COMMON_DIR)/cpu.h
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR)/memory.o: $(COMMON_DIR)/memory.c $(COMMON_DIR)/memory.h $(COMMON_DIR)/gameboy.h
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR)/screen.o: $(COMMON_DIR)/screen.c $(COMMON_DIR)/screen.h
	$(CC) -c $(CFLAGS) $< -o $@


# Link
$(BIN_DIR)/$(EXECUTABLE): $(OBJ_DIR)/$(MAIN).o $(OBJ_DIR)/gameboy.o $(OBJ_DIR)/cpu.o $(OBJ_DIR)/memory.o $(OBJ_DIR)/screen.o
	$(CC) $(CFLAGS) $^ -o $@ -lm

# Copy bootloader rom.
$(BIN_DIR)/DMG_ROM.bin: DMG_ROM.bin
	$(COPY_BTLDR_CMD)

# Target: clean project.
.PHONY: clean
clean: 
	$(CLEAN_CMD)



