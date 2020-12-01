#ifndef SRC_GAMEBOY_H_
#define SRC_GAMEBOY_H_

#include <stdint.h>
#include <stdio.h>
#include "cpu.h"

typedef struct gameboy_t {
    CPU* cpu;
    uint8_t* memory;
    uint8_t* bootstrap_rom;
    uint8_t int_master_enable;

    uint32_t timer_counter;
    uint32_t divider_counter;
} Gameboy;

uint8_t gameboy_execute_instruction(Gameboy* gb, uint8_t instruction);
uint8_t gameboy_execute_cb_prefix_instruction(Gameboy* gb, uint8_t base);

void gameboy_load_bootstrap(Gameboy* gb, FILE* fp);
void gameboy_load_rom(Gameboy* gb, FILE* fp);
void gameboy_execution_loop(Gameboy* gb);
void gameboy_update_buttons(Gameboy* gb, uint8_t buttons);
void gameboy_update(Gameboy* gb);
void gameboy_single_frame_update(Gameboy* gb, uint8_t buttons, uint8_t* frame_buffer);


#endif  // SRC_GAMEBOY_H_