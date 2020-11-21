#include <stdint.h>
#include "memory.h"
#include "gameboy.h"

uint8_t memory_get8(Gameboy* gb, uint16_t address) {
    if (gb->memory[0xFF50] || address >= 0x100) {
        return gb->memory[address];
    } else {
        return gb->bootstrap_rom[address];
    }
}

void memory_set8(Gameboy* gb, uint16_t address, uint8_t value) {
    if (address >=0xFE00 && address <= 0xFE9F) {
        printf("yay? address: 0x%.4x value: $%.2x\n", address, value);
    }
    if (address == 0xFF00) {
        // Prevent buttons being overwritten.
        gb->memory[address] = (value & 0xF0) | (gb->memory[address] & 0x0F);
    } else {
        gb->memory[address] = value;
    }
}

uint16_t memory_get16(Gameboy* gb, uint16_t address) {
    if (gb->memory[0xFF50] || address >= 0x100) {
        return (gb->memory[address+1] << 8) | gb->memory[address];
    } else {
        return (gb->bootstrap_rom[address+1] << 8) | gb->bootstrap_rom[address];
    }
}

void memory_set16(Gameboy* gb, uint16_t address, uint16_t value) {
    memory_set8(gb, address, value & 0xFF);
    memory_set8(gb, address+1, value >> 8);
    // gb->memory[address] = value & 0xFF;
    // gb->memory[address+1] = value >> 8;
}



