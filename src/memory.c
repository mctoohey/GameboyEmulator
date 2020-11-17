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
    gb->memory[address] = value;
}

uint16_t memory_get16(Gameboy* gb, uint16_t address) {
    if (gb->memory[0xFF50] || address >= 0x100) {
        return (gb->memory[address+1] << 8) | gb->memory[address];
    } else {
        return (gb->bootstrap_rom[address+1] << 8) | gb->bootstrap_rom[address];
    }
}

void memory_set16(Gameboy* gb, uint16_t address, uint16_t value) {
    gb->memory[address] = value & 0xFF;
    gb->memory[address+1] = value >> 8;
}



