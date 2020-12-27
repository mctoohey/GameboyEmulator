#include "memory.h"

#include <stdint.h>

#include "gameboy.h"
#include "mbc_struct.h"


void memory_dma_transfer(Gameboy* gb, uint8_t value) {
    uint16_t address = value << 8;
    for (uint8_t i = 0; i < 0xA0; i++) {
        memory_set8(gb, 0xFE00+i, memory_get8(gb, address+i));
    }
}


void memory_do_banking(Gameboy* gb, uint16_t address, uint8_t value) {
    // Enable/Disable writing to RAM bank.
    if (address < 0x2000 && (gb->mbc_type == MBC1 || gb->mbc_type == MBC2)) {
        if (gb->mbc_type == MBC2 && (address & (1 << 4))) {
            return;
        } else if ((value & 0xF) == 0xA) {
            gb->ram_bank_writable = 1;
        } else if ((value & 0xF) == 0x0) {
            gb->ram_bank_writable = 0;
        }
    // Change upper ROM bank bits.
    } else if (address >= 0x200 && address < 0x4000 && (gb->mbc_type == MBC1 || gb->mbc_type == MBC2)) {
        if (gb->mbc_type == MBC2) {
            gb->current_cartridge_bank = value & 0xF;
            if (gb->current_cartridge_bank == 0) gb->current_cartridge_bank = 1;
            return;
        }

        gb->current_cartridge_bank &= 0xE0;     // Set lower 5 bits to zero.
        gb->current_cartridge_bank |= (value & 0x1F);
        if (gb->current_cartridge_bank == 0) gb->current_cartridge_bank = 1;
    // Change lower ROM bank bits or RAM bank.
    } else if (address >= 0x4000 && address < 0x6000 && (gb->mbc_type == MBC1)) {
        if (gb->doing_rom_banking) {
            gb->current_cartridge_bank &= 0x1F;  // Set upper 3 bits to zero.
            gb->current_cartridge_bank |= (value & 0xE0);
            if (gb->current_cartridge_bank == 0) gb->current_cartridge_bank = 1;
        } else {
            gb->current_ram_bank = value & 0x3;
        }
    // Change banking mode.
    } else if (address >= 0x6000 && address < 0x8000 && (gb->mbc_type == MBC1)) {
        if ((value & 0x1) == 0) {
            gb->doing_rom_banking = 1;
            gb->current_ram_bank = 0;
        } else {
            gb->doing_rom_banking = 0;
        }
    }
}


uint8_t memory_get8(Gameboy* gb, uint16_t address) {
    if (address < 0x4000) {
        if (gb->memory[0xFF50] || address >= 0x100) {
            return gb->cartridge_rom[address];
        } else {
            return gb->bootstrap_rom[address];
        }
    } else if (address < 0x8000) {
        return gb->cartridge_rom[address-0x4000 + (gb->current_cartridge_bank*0x4000)];
    } else if (address >= 0xA000 && address < 0xC000) {
        return gb->ram_banks[address-0xA000 + (gb->current_ram_bank*0x2000)];
    } else {
        return gb->memory[address];
    }
}

void memory_set8(Gameboy* gb, uint16_t address, uint8_t value) {
    if (address < 0x8000) {
        memory_do_banking(gb, address, value);
    } else if (address >= 0xA000 && address < 0xC000 && gb->ram_bank_writable) {
        gb->ram_banks[address-0xA000 + (gb->current_ram_bank*0x2000)] = value;
    } else if (address == 0xFF00) {
        // Prevent buttons being overwritten.
        gb->memory[address] = (value & 0xF0) | (gb->memory[address] & 0x0F);
    } else if (address == 0xFF46) {
        memory_dma_transfer(gb, value);
    } else if (address = 0xFF44) {
        gb->memory[0xFF44] = 0;     // Reset scanline.
    } else {
        gb->memory[address] = value;
    }
}

uint16_t memory_get16(Gameboy* gb, uint16_t address) {
    return (memory_get8(gb, address+1) << 8) | memory_get8(gb, address);
}

void memory_set16(Gameboy* gb, uint16_t address, uint16_t value) {
    memory_set8(gb, address, value & 0xFF);
    memory_set8(gb, address+1, value >> 8);
    // gb->memory[address] = value & 0xFF;
    // gb->memory[address+1] = value >> 8;
}




