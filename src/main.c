#include <stdio.h>
#include <stdint.h>

#include "cpu.h"
#include "gameboy.h"

int main(void) {
    printf("hi\n");
    CPU cpu;
    cpu.PC = 0;
    uint8_t memory[0x10000] = {0};
    uint8_t bootstrap_rom[0x100] = {0};
    Gameboy gb = {&cpu, memory, bootstrap_rom};

    FILE* rom_fp = fopen("../../ROMS/tetris.gb", "rb");
    FILE* boostrap_fp = fopen("DMG_ROM.bin", "rb");

    gameboy_load_rom(&gb, rom_fp);
    gameboy_load_bootstrap(&gb, boostrap_fp);

    fclose(rom_fp);
    fclose(boostrap_fp);

    memory[0xFF44] = 0x90;

    gameboy_execution_loop(&gb);
}
