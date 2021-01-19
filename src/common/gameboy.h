#ifndef SRC_GAMEBOY_H_
#define SRC_GAMEBOY_H_

#include <stdint.h>
#include <stdio.h>
#include "cpu.h"
#include "mbc_struct.h"

/** Struct that stores the state of the gameboy. */
typedef struct gameboy_t {
    CPU* cpu;
    uint8_t* memory;
    uint8_t* ram_banks;
    uint8_t* bootstrap_rom;
    uint8_t* cartridge_rom;
    uint8_t current_cartridge_bank;
    uint8_t current_ram_bank;

    enum MBCType mbc_type;
    uint8_t ram_bank_writable;
    uint8_t doing_rom_banking;

    uint8_t int_master_enable;

    uint32_t timer_counter;
    uint32_t divider_counter;
} Gameboy;

/** Gets the next instruction pointed to by PC.
 *
 * @param gb Gameboy to operate on.
 * @return op code of the instruction.
*/
uint8_t gameboy_execute_instruction(Gameboy* gb, uint8_t instruction);

/** Executes a instruction with CB prefix.
 *  @param gb Gameboy to execute the instruction on.
 *  @param base The 'base' of the instruction (byte after CB prefix).
 *  
 * @return The number of cpu cycles the instruction took to execute.
*/
uint8_t gameboy_execute_cb_prefix_instruction(Gameboy* gb, uint8_t base);

/** Allocates and creates a new Gameboy struct.
 *  
 * @return A pointer to the Gameboy struct created.
*/
Gameboy* gameboy_create(void);

/** Frees all memory used by the Gameboy.
 *  
 * @param gb Gameboy to destroy.
 * @return A pointer to the Gameboy struct created.
*/
void gameboy_destroy(Gameboy* gb);

/** Loads the bootstrap from specified file into the gameboy emulator's bootstrap ROM memory.
 * 
 * @param gb Gameboy to operate on.
 * @param fp Pointer to the FILE to load the bootstrap from.
*/
void gameboy_load_bootstrap(Gameboy* gb, FILE* fp);

/** Loads a ROM from the specified file into the gameboy emulator's cartridge ROM memory.
 *
 * @param gb Gameboy to operate on.
 * @param fp Pointer to the File to load the cartridge ROM from.
*/
void gameboy_load_rom(Gameboy* gb, FILE* fp);

/** Enter an infinte loop that reads and executes instructions from the ROM.
 *  Should only be used for testing, does not handle timers, interrupts or display.
 *
 *  @param gb The Gameboy to operate on.
*/
void gameboy_execution_loop(Gameboy* gb);
void gameboy_update_buttons(Gameboy* gb, uint8_t buttons);
void gameboy_update(Gameboy* gb);
void gameboy_single_frame_update(Gameboy* gb, uint8_t buttons, uint8_t* frame_buffer);


#endif  // SRC_GAMEBOY_H_
