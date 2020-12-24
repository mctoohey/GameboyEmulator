#include "gameboy.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "instructions.h"
#include "logging.h"
#include "memory.h"
#include "screen.h"


#define CYCLES_PER_FRAME CPU_FREQUENCY/60
#define CYCLES_PER_LINE CYCLES_PER_FRAME/154

#define BYTES_PER_BANK 0x4000

static const uint16_t interrupt_vector[5] = {
    0x0040,
    0x0048,
    0x0050,
    0x0058,
    0x0060
};

static const uint16_t timer_thresholds[4] = {
    CPU_FREQUENCY/4096,
    CPU_FREQUENCY/262144,
    CPU_FREQUENCY/65536,
    CPU_FREQUENCY/16384,
};

/** Allocates and creates a new Gameboy struct.
 *  
 * @return A pointer to the Gameboy struct created.
*/
Gameboy* gameboy_create(void) {
    Gameboy* gb = malloc(sizeof(Gameboy));

    gb->cpu = malloc(sizeof(CPU));
    gb->cpu->PC = 0;

    gb->memory = malloc(0x10000);
    gb->bootstrap_rom = malloc(0x100);
    gb->ram_banks = malloc(0x8000);
    gb->cartridge_rom = NULL;

    gb->mbc_type = ROM_ONLY;
    gb->ram_bank_writable = 0;
    gb->current_cartridge_bank = 1;
    gb->doing_rom_banking = 1;
    gb->current_ram_bank = 0;
    gb->int_master_enable = 0;
    gb->timer_counter = 0;
    gb->divider_counter = 0;
    return gb;
}

/** Frees all memory used by the Gameboy.
 *  
 * @param gb Gameboy to destroy.
 * @return A pointer to the Gameboy struct created.
*/
void gameboy_destroy(Gameboy* gb) {
    free(gb->cpu);
    free(gb->memory);
    free(gb->bootstrap_rom);
    free(gb->cartridge_rom);

    free(gb);
}

uint8_t gameboy_fetch_instruction(Gameboy* gb) {
    return memory_get8(gb, gb->cpu->PC++);
}

uint8_t gameboy_fetch_immediate8(Gameboy* gb) {
    return memory_get8(gb, gb->cpu->PC++);
}

uint16_t gameboy_fetch_immediate16(Gameboy* gb) {
    gb->cpu->PC += 2;
    return memory_get16(gb, gb->cpu->PC-2);
}


void gameboy_push16(Gameboy* gb, uint16_t value) {
    gb->cpu->SP -= 2;
    memory_set16(gb, gb->cpu->SP, value);
}

uint16_t gameboy_pop16(Gameboy* gb) {
    uint16_t value = memory_get16(gb, gb->cpu->SP);
    gb->cpu->SP += 2;
    return value;
}


void gameboy_service_interrupt(Gameboy* gb, uint16_t routine_address) {
    gameboy_push16(gb, gb->cpu->PC);
    gb->cpu->PC = routine_address;
}


void gameboy_check_interrupts(Gameboy* gb) {
    if (!gb->int_master_enable) {
        return;
    }

    for (uint8_t i = 0; i < 5; i++) {
        if (gb->memory[0xFFFF] & gb->memory[0xFF0F] & (1 << i)) {
            LOG_DEBUG("Interupt %d", i);
            gb->memory[0xFF0F] &= ~(1 << i);  // Reset
            gb->int_master_enable = 0;
            gameboy_service_interrupt(gb, interrupt_vector[i]);
            break;
        }
    }
}


void gameboy_load_bootstrap(Gameboy* gb, FILE* fp) {
    fread(gb->bootstrap_rom, 1, 0x100, fp);
}


void gameboy_load_rom(Gameboy* gb, FILE* fp) {
    free(gb->cartridge_rom);
    gb->cartridge_rom = malloc(0x8000);
    fread(gb->cartridge_rom, 1, 0x8000, fp);

    uint32_t rom_size;
    switch (gb->cartridge_rom[0x148]) {
        case 0x00:
            rom_size = 2*BYTES_PER_BANK;
            break;
        case 0x01:
            rom_size = 4*BYTES_PER_BANK;
            break;
        case 0x02:
            rom_size = 8*BYTES_PER_BANK;
            break;
        case 0x03:
            rom_size = 16*BYTES_PER_BANK;
            break;
        case 0x04:
            rom_size = 32*BYTES_PER_BANK;
            break;
        case 0x05:
            rom_size = 64*BYTES_PER_BANK;
            break;
        case 0x06:
            rom_size = 128*BYTES_PER_BANK;
            break;
        case 0x52:
            rom_size = 72*BYTES_PER_BANK;
            break;
        case 0x53:
            rom_size = 80*BYTES_PER_BANK;
            break;
        case 0x54:
            rom_size = 96*BYTES_PER_BANK;
            break;
        default:
            LOG_ERROR("Invalid ROM size 0x%.2X", gb->cartridge_rom[0x148]);
            exit(1);
    }

    gb->cartridge_rom = realloc(gb->cartridge_rom, rom_size);
    fread(gb->cartridge_rom+0x8000, 1, rom_size-0x8000, fp);

    switch (gb->cartridge_rom[0x147]) {
        case 0x00:
            gb->mbc_type = ROM_ONLY;
            break;
        case 0x01:
        case 0x02:
        case 0x03:
            gb->mbc_type = MBC1;
            break;
        case 0x05:
        case 0x06:
            gb->mbc_type = MBC2;
            break;
        default:
            LOG_ERROR("Cartridge type 0x%.2X not implemented or not valid.", gb->cartridge_rom[0x147]);
            exit(1);

    }
}


void gameboy_execution_loop(Gameboy* gb) {
    uint8_t instruction;
    uint32_t i = 0;
    uint32_t count = 0;
    while (1) {
        LOG_DEBUG("PC = $%.4x", gb->cpu->PC);
        instruction = gameboy_fetch_immediate8(gb);
        gameboy_execute_instruction(gb, instruction);
        if (i >= count) {
            printf("DONE!\n");
            scanf("%u", &count);
            i = 0;
        }
        i += 1;
    }
}


void gameboy_update_buttons(Gameboy* gb, uint8_t buttons) {
    // TODO(mct): Only trigger interrupt on change in edge (hi to low).
    if (buttons != 0xFF) {
        gb->memory[0xFF0F] |= (1 << 4);   // Set interrupt
    }

    if (!(gb->memory[0xFF00] & (1 << 5))) {
        gb->memory[0xFF00] = (gb->memory[0xFF00] & 0xF0) | (buttons & 0x0F);
    } else if (!(gb->memory[0xFF00] & (1 << 4))) {
        gb->memory[0xFF00] = (gb->memory[0xFF00] & 0xF0) | (buttons >> 4);
    }
}

// void gameboy_update_timer(Gameboy* gb) {

// }


void gameboy_update(Gameboy* gb) {
    LOG_DEBUG("PC = $%.4x", gb->cpu->PC);

    uint8_t instruction = gameboy_fetch_immediate8(gb);
    gameboy_execute_instruction(gb, instruction);

    gameboy_check_interrupts(gb);
}

void gameboy_single_frame_update(Gameboy* gb, uint8_t buttons, uint8_t* frame_buffer) {

    for (uint16_t j = 0; j < 154; j++) {
        uint32_t cycles = 0;

        while (cycles < CYCLES_PER_LINE) {
            gameboy_update_buttons(gb, buttons);  // TODO(mct): Remove

            uint8_t instruction = gameboy_fetch_immediate8(gb);
            uint8_t instruction_cycles = gameboy_execute_instruction(gb, instruction);
            cycles += instruction_cycles;
            gb->timer_counter += instruction_cycles;
            gb->divider_counter += instruction_cycles;

            // Update timer.
            if (gb->timer_counter >= timer_thresholds[gb->memory[0xFF07]]) {
                gb->memory[0xFF05]++;
                if (gb->memory[0xFF05] == 0) {
                    gb->memory[0xFF0F] |= (1 << 2);     // Trigger Interrupt.
                    gb->memory[0xFF05] = gb->memory[0xFF06];
                }
                gb->timer_counter = 0;
            }

            // Update divider register.
            if (gb->divider_counter >= CPU_FREQUENCY/16382) {
                gb->memory[0xFF04]++;
                gb->divider_counter = 0;
            }

            gameboy_check_interrupts(gb);


        }
        screen_scanline_update(gb->memory, frame_buffer);
    }
}


/** Executes a single instruction.
 *  @param gb Gameboy to execute the instruction on.
 *  @param instruction The opcode of the instruction to execute.
 *  
 * @return The number of cpu cycles the instruction took to execute.
*/
uint8_t gameboy_execute_instruction(Gameboy* gb, uint8_t instruction) {
    uint8_t cycles = 0;
    switch (instruction) {
        case LD_A_d8:
            LOG_INFO("LD A,d8");
            gb->cpu->A = gameboy_fetch_immediate8(gb);
            cycles = 8;
            break;
        case LD_B_d8:
            LOG_INFO("LD B,d8");
            gb->cpu->B = gameboy_fetch_immediate8(gb);
            cycles = 8;
            break;
        case LD_C_d8:
            LOG_INFO("LD C,d8");
            gb->cpu->C = gameboy_fetch_immediate8(gb);
            cycles = 8;
            break;
        case LD_D_d8:
            LOG_INFO("LD D,d8");
            gb->cpu->D = gameboy_fetch_immediate8(gb);
            cycles = 8;
            break;
        case LD_E_d8:
            LOG_INFO("LD E,d8");
            gb->cpu->E = gameboy_fetch_immediate8(gb);
            cycles = 8;
            break;
        case LD_H_d8:
            LOG_INFO("LD H,d8");
            gb->cpu->H = gameboy_fetch_immediate8(gb);
            cycles = 8;
            break;
        case LD_L_d8:
            LOG_INFO("LD L,d8");
            gb->cpu->L = gameboy_fetch_immediate8(gb);
            cycles = 8;
            break;

        case LD_A_A:
            LOG_INFO("LD A,A");
            gb->cpu->A = gb->cpu->A;
            cycles = 4;
            break;
        case LD_A_B:
            LOG_INFO("LD A,B");
            gb->cpu->A = gb->cpu->B;
            cycles = 4;
            break;
        case LD_A_C:
            LOG_INFO("LD A,C");
            gb->cpu->A = gb->cpu->C;
            cycles = 4;
            break;
        case LD_A_D:
            LOG_INFO("LD A,D");
            gb->cpu->A = gb->cpu->D;
            cycles = 4;
            break;
        case LD_A_E:
            LOG_INFO("LD A,E");
            gb->cpu->A = gb->cpu->E;
            cycles = 4;
            break;
        case LD_A_H:
            LOG_INFO("LD A,H");
            gb->cpu->A = gb->cpu->H;
            cycles = 4;
            break;
        case LD_A_L:
            LOG_INFO("LD A,L");
            gb->cpu->A = gb->cpu->L;
            cycles = 4;
            break;

        case LD_B_A:
            LOG_INFO("LD B,A");
            gb->cpu->B = gb->cpu->A;
            cycles = 4;
            break;
        case LD_B_B:
            LOG_INFO("LD B,B");
            gb->cpu->B = gb->cpu->B;
            cycles = 4;
            break;
        case LD_B_C:
            LOG_INFO("LD B,C");
            gb->cpu->B = gb->cpu->C;
            cycles = 4;
            break;
        case LD_B_D:
            LOG_INFO("LD B,D");
            gb->cpu->B = gb->cpu->D;
            cycles = 4;
            break;
        case LD_B_E:
            LOG_INFO("LD B,E");
            gb->cpu->B = gb->cpu->E;
            cycles = 4;
            break;
        case LD_B_H:
            LOG_INFO("LD B,H");
            gb->cpu->B = gb->cpu->H;
            cycles = 4;
            break;
        case LD_B_L:
            LOG_INFO("LD B,L");
            gb->cpu->B = gb->cpu->L;
            cycles = 4;
            break;

        case LD_C_A:
            LOG_INFO("LD C,A");
            gb->cpu->C = gb->cpu->A;
            cycles = 4;
            break;
        case LD_C_B:
            LOG_INFO("LD C,B");
            gb->cpu->C = gb->cpu->B;
            cycles = 4;
            break;
        case LD_C_C:
            LOG_INFO("LD C,C");
            gb->cpu->C = gb->cpu->C;
            cycles = 4;
            break;
        case LD_C_D:
            LOG_INFO("LD C,D");
            gb->cpu->C = gb->cpu->D;
            cycles = 4;
            break;
        case LD_C_E:
            LOG_INFO("LD C,E");
            gb->cpu->C = gb->cpu->E;
            cycles = 4;
            break;
        case LD_C_H:
            LOG_INFO("LD C,H");
            gb->cpu->C = gb->cpu->H;
            cycles = 4;
            break;
        case LD_C_L:
            LOG_INFO("LD C,L");
            gb->cpu->C = gb->cpu->L;
            cycles = 4;
            break;

        case LD_D_A:
            LOG_INFO("LD D,A");
            gb->cpu->D = gb->cpu->A;
            cycles = 4;
            break;
        case LD_D_B:
            LOG_INFO("LD D,B");
            gb->cpu->D = gb->cpu->B;
            cycles = 4;
            break;
        case LD_D_C:
            LOG_INFO("LD D,C");
            gb->cpu->D = gb->cpu->C;
            cycles = 4;
            break;
        case LD_D_D:
            LOG_INFO("LD D,D");
            gb->cpu->D = gb->cpu->D;
            cycles = 4;
            break;
        case LD_D_E:
            LOG_INFO("LD D,E");
            gb->cpu->D = gb->cpu->E;
            cycles = 4;
            break;
        case LD_D_H:
            LOG_INFO("LD D,H");
            gb->cpu->D = gb->cpu->H;
            cycles = 4;
            break;
        case LD_D_L:
            LOG_INFO("LD D,L");
            gb->cpu->D = gb->cpu->L;
            cycles = 4;
            break;

        case LD_E_A:
            LOG_INFO("LD E,A");
            gb->cpu->E = gb->cpu->A;
            cycles = 4;
            break;
        case LD_E_B:
            LOG_INFO("LD E,B");
            gb->cpu->E = gb->cpu->B;
            cycles = 4;
            break;
        case LD_E_C:
            LOG_INFO("LD E,C");
            gb->cpu->E = gb->cpu->C;
            cycles = 4;
            break;
        case LD_E_D:
            LOG_INFO("LD E,D");
            gb->cpu->E = gb->cpu->D;
            cycles = 4;
            break;
        case LD_E_E:
            LOG_INFO("LD E,E");
            gb->cpu->E = gb->cpu->E;
            cycles = 4;
            break;
        case LD_E_H:
            LOG_INFO("LD E,H");
            gb->cpu->E = gb->cpu->H;
            cycles = 4;
            break;
        case LD_E_L:
            LOG_INFO("LD E,L");
            gb->cpu->E = gb->cpu->L;
            cycles = 4;
            break;

        case LD_H_A:
            LOG_INFO("LD H,A");
            gb->cpu->H = gb->cpu->A;
            cycles = 4;
            break;
        case LD_H_B:
            LOG_INFO("LD H,B");
            gb->cpu->H = gb->cpu->B;
            cycles = 4;
            break;
        case LD_H_C:
            LOG_INFO("LD H,C");
            gb->cpu->H = gb->cpu->C;
            cycles = 4;
            break;
        case LD_H_D:
            LOG_INFO("LD H,D");
            gb->cpu->H = gb->cpu->D;
            cycles = 4;
            break;
        case LD_H_E:
            LOG_INFO("LD H,E");
            gb->cpu->H = gb->cpu->E;
            cycles = 4;
            break;
        case LD_H_H:
            LOG_INFO("LD H,H");
            gb->cpu->H = gb->cpu->H;
            cycles = 4;
            break;
        case LD_H_L:
            LOG_INFO("LD H,L");
            gb->cpu->H = gb->cpu->L;
            cycles = 4;
            break;

        case LD_L_A:
            LOG_INFO("LD L,A");
            gb->cpu->L = gb->cpu->A;
            cycles = 4;
            break;
        case LD_L_B:
            LOG_INFO("LD L,B");
            gb->cpu->L = gb->cpu->B;
            cycles = 4;
            break;
        case LD_L_C:
            LOG_INFO("LD L,C");
            gb->cpu->L = gb->cpu->C;
            cycles = 4;
            break;
        case LD_L_D:
            LOG_INFO("LD L,D");
            gb->cpu->L = gb->cpu->D;
            cycles = 4;
            break;
        case LD_L_E:
            LOG_INFO("LD L,E");
            gb->cpu->L = gb->cpu->E;
            cycles = 4;
            break;
        case LD_L_H:
            LOG_INFO("LD L,H");
            gb->cpu->L = gb->cpu->H;
            cycles = 4;
            break;
        case LD_L_L:
            LOG_INFO("LD L,L");
            gb->cpu->L = gb->cpu->L;
            cycles = 4;
            break;

        case LD_A_BC:
            LOG_INFO("LD A,(BC)");
            gb->cpu->A = memory_get8(gb, cpu_get_value_BC(gb->cpu));
            cycles = 8;
            break;
        case LD_A_DE:
            LOG_INFO("LD A,(DE)");
            gb->cpu->A = memory_get8(gb, cpu_get_value_DE(gb->cpu));
            cycles = 8;
            break;
        case LD_A_HL:
            LOG_INFO("LD A,(HL)");
            gb->cpu->A = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cycles = 8;
            break;
        case LD_B_HL:
            LOG_INFO("LD B,(HL)");
            gb->cpu->B = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cycles = 8;
            break;
        case LD_C_HL:
            LOG_INFO("LD C,(HL)");
            gb->cpu->C = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cycles = 8;
            break;
        case LD_D_HL:
            LOG_INFO("LD D,(HL)");
            gb->cpu->D = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cycles = 8;
            break;
        case LD_E_HL:
            LOG_INFO("LD E,(HL)");
            gb->cpu->E = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cycles = 8;
            break;
        case LD_H_HL:
            LOG_INFO("LD H,(HL)");
            gb->cpu->H = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cycles = 8;
            break;
        case LD_L_HL:
            LOG_INFO("LD L,(HL)");
            gb->cpu->L = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cycles = 8;
            break;

        case LD_BC_A:
            LOG_INFO("LD (BC),A");
            memory_set8(gb, cpu_get_value_BC(gb->cpu), gb->cpu->A);
            cycles = 8;
            break;
        case LD_DE_A:
            LOG_INFO("LD (DE),A");
            memory_set8(gb, cpu_get_value_DE(gb->cpu), gb->cpu->A);
            cycles = 8;
            break;
        case LD_HL_A:
            LOG_INFO("LD (HL),A");
            memory_set8(gb, cpu_get_value_HL(gb->cpu), gb->cpu->A);
            cycles = 8;
            break;
        case LD_HL_B:
            LOG_INFO("LD (HL),B");
            memory_set8(gb, cpu_get_value_HL(gb->cpu), gb->cpu->B);
            cycles = 8;
            break;
        case LD_HL_C:
            LOG_INFO("LD (HL),C");
            memory_set8(gb, cpu_get_value_HL(gb->cpu), gb->cpu->C);
            cycles = 8;
            break;
        case LD_HL_D:
            LOG_INFO("LD (HL),D");
            memory_set8(gb, cpu_get_value_HL(gb->cpu), gb->cpu->D);
            cycles = 8;
            break;
        case LD_HL_E:
            LOG_INFO("LD (HL),E");
            memory_set8(gb, cpu_get_value_HL(gb->cpu), gb->cpu->E);
            cycles = 8;
            break;
        case LD_HL_H:
            LOG_INFO("LD (HL),H");
            memory_set8(gb, cpu_get_value_HL(gb->cpu), gb->cpu->H);
            cycles = 8;
            break;
        case LD_HL_L:
            LOG_INFO("LD (HL),L");
            memory_set8(gb, cpu_get_value_HL(gb->cpu), gb->cpu->L);
            cycles = 8;
            break;

        case LD_HL_d8:
            LOG_INFO("LD (HL),d8");
            memory_set8(gb, cpu_get_value_HL(gb->cpu), gameboy_fetch_immediate8(gb));
            cycles = 12;
            break;

        case LD_A_a16:
            LOG_INFO("LD A,(a16)");
            gb->cpu->A = memory_get16(gb, gameboy_fetch_immediate16(gb));
            cycles = 16;
            break;

        case LD_a16_A:
            LOG_INFO("LD (a16),A");
            memory_set8(gb, gameboy_fetch_immediate16(gb), gb->cpu->A);
            cycles = 16;
            break;

        case LD_A_addrC:
            LOG_INFO("LD A,(0xFF00+C)");
            gb->cpu->A = memory_get8(gb, 0xFF00 | gb->cpu->C);
            cycles = 8;
            break;

        case LD_addrC_A:
            LOG_INFO("LD (0xFF00+C),A");
            memory_set8(gb, 0xFF00 | gb->cpu->C, gb->cpu->A);
            cycles = 8;
            break;

        case LDD_A_HL:
            LOG_INFO("LDD A,(HL)");
            gb->cpu->A = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cpu_decrement_HL(gb->cpu);
            cycles = 8;
            break;

        case LDD_HL_A:
            LOG_INFO("LDD (HL),A");
            memory_set8(gb, cpu_get_value_HL(gb->cpu), gb->cpu->A);
            cpu_decrement_HL(gb->cpu);
            LOG_DEBUG("A = 0x%.2x", gb->cpu->A);
            LOG_DEBUG("HL = 0x%.4x", cpu_get_value_HL(gb->cpu));
            cycles = 8;
            break;

        case LDI_A_HL:
            LOG_INFO("LDI A,(HL)");
            gb->cpu->A = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cpu_increment_HL(gb->cpu);
            cycles = 8;
            break;

        case LDI_HL_A:
            LOG_INFO("LDI (HL),A");
            memory_set8(gb, cpu_get_value_HL(gb->cpu), gb->cpu->A);
            cpu_increment_HL(gb->cpu);
            cycles = 8;
            break;

        case LDH_a8_A:
            LOG_INFO("LDH (a8),A");
            memory_set8(gb, 0xFF00 | gameboy_fetch_immediate8(gb), gb->cpu->A);
            cycles = 12;
            break;

        case LDH_A_a8:
            LOG_INFO("LDH A,(a8)");
            uint16_t address = 0xFF00 | gameboy_fetch_immediate8(gb);
            LOG_DEBUG("Address = 0x%.4X", address);
            gb->cpu->A = memory_get8(gb, address);
            LOG_DEBUG("A = $%.2x", gb->cpu->A);
            cycles = 12;
            break;



        // 16 bit loads.
        case LD_BC_d16:
            LOG_INFO("LD BC,d16");
            cpu_set_value_BC(gb->cpu, gameboy_fetch_immediate16(gb));
            cycles = 12;
            break;
        case LD_DE_d16:
            LOG_INFO("LD DE,d16");
            cpu_set_value_DE(gb->cpu, gameboy_fetch_immediate16(gb));
            cycles = 12;
            break;
        case LD_HL_d16:
        {
            LOG_INFO("LD HL,d16");
            uint16_t value = gameboy_fetch_immediate16(gb);
            LOG_DEBUG("Fetched 0x%.4x", value);
            cpu_set_value_HL(gb->cpu, value);
            LOG_DEBUG("HL = 0x%.4x", cpu_get_value_HL(gb->cpu));
            cycles = 12;
            break;
        }
        case LD_SP_d16:
            LOG_INFO("LD SP,d16");
            gb->cpu->SP = gameboy_fetch_immediate16(gb);
            cycles = 12;
            break;

        case LD_SP_HL:
            LOG_INFO("LD SP,HL");
            gb->cpu->SP = cpu_get_value_HL(gb->cpu);
            cycles = 8;
            break;

        case LDHL_SP_d8:
        {
            LOG_INFO("LD HL,SP+d8");
            uint8_t value = gameboy_fetch_immediate8(gb);
            (gb->cpu->SP & 0x0F) + (value & 0x0F) > 0x0F ? cpu_flag_setH(gb->cpu) : cpu_flag_resetH(gb->cpu);
            (gb->cpu->SP & 0xFF) + value > 0xFF ? cpu_flag_setC(gb->cpu) : cpu_flag_resetC(gb->cpu);
            cpu_set_value_HL(gb->cpu, gb->cpu->SP + ((int8_t) value));
            cpu_flag_resetZ(gb->cpu);
            cpu_flag_resetN(gb->cpu);
            cycles = 12;
        }
            break;

        case LD_a16_SP:
            LOG_INFO("LD (a16),SP");
            memory_set16(gb, gameboy_fetch_immediate16(gb), gb->cpu->SP);
            cycles = 20;
            break;

        // PUSH Instructions.
        case PUSH_AF:
            LOG_INFO("PUSH AF");
            gameboy_push16(gb, cpu_get_value_AF(gb->cpu));
            cycles = 16;
            break;
        case PUSH_BC:
            LOG_INFO("PUSH BC");
            LOG_DEBUG("Pushing value 0x%.4x", cpu_get_value_BC(gb->cpu));
            gameboy_push16(gb, cpu_get_value_BC(gb->cpu));
            cycles = 16;
            break;
        case PUSH_DE:
            LOG_INFO("PUSH DE");
            gameboy_push16(gb, cpu_get_value_DE(gb->cpu));
            cycles = 16;
            break;
        case PUSH_HL:
            LOG_INFO("PUSH HL");
            gameboy_push16(gb, cpu_get_value_HL(gb->cpu));
            cycles = 16;
            break;

        // POP Instructions.
        case POP_AF:
            LOG_INFO("POP AF");
            cpu_set_value_AF(gb->cpu, gameboy_pop16(gb));
            cycles = 12;
            break;
        case POP_BC:
            LOG_INFO("POP BC");
            cpu_set_value_BC(gb->cpu, gameboy_pop16(gb));
            LOG_DEBUG("Popped value 0x%.4x", cpu_get_value_BC(gb->cpu));
            cycles = 12;
            break;
        case POP_DE:
            LOG_INFO("POP DE");
            cpu_set_value_DE(gb->cpu, gameboy_pop16(gb));
            cycles = 12;
            break;
        case POP_HL:
            LOG_INFO("POP HL");
            cpu_set_value_HL(gb->cpu, gameboy_pop16(gb));
            cycles = 12;
            break;



        // 8 bit ALU instructions.

        // 8 bit add instructions.
        case ADD_A_A:
            LOG_INFO("ADD A,A");
            cpu_add_to_A(gb->cpu, gb->cpu->A);
            cycles = 4;
            break;
        case ADD_A_B:
            LOG_INFO("ADD A,B");
            cpu_add_to_A(gb->cpu, gb->cpu->B);
            cycles = 4;
            break;
        case ADD_A_C:
            LOG_INFO("ADD A,C");
            cpu_add_to_A(gb->cpu, gb->cpu->C);
            cycles = 4;
            break;
        case ADD_A_D:
            LOG_INFO("ADD A,D");
            cpu_add_to_A(gb->cpu, gb->cpu->D);
            cycles = 4;
            break;
        case ADD_A_E:
            LOG_INFO("ADD A,E");
            cpu_add_to_A(gb->cpu, gb->cpu->E);
            cycles = 4;
            break;
        case ADD_A_H:
            LOG_INFO("ADD A,H");
            cpu_add_to_A(gb->cpu, gb->cpu->H);
            cycles = 4;
            break;
        case ADD_A_L:
            LOG_INFO("ADD A,L");
            cpu_add_to_A(gb->cpu, gb->cpu->L);
            cycles = 4;
            break;

        case ADD_A_HL:
            LOG_INFO("ADD A,(HL)");
            cpu_add_to_A(gb->cpu, memory_get8(gb, cpu_get_value_HL(gb->cpu)));
            cycles = 8;
            break;

        case ADD_A_d8:
            LOG_INFO("ADD A,d8");
            cpu_add_to_A(gb->cpu, gameboy_fetch_immediate8(gb));
            cycles = 8;
            break;

        case ADC_A_A:
            LOG_INFO("ADC A,A");
            cpu_addcarry_to_A(gb->cpu, gb->cpu->A);
            cycles = 4;
            break;
        case ADC_A_B:
            LOG_INFO("ADC A,B");
            cpu_addcarry_to_A(gb->cpu, gb->cpu->B);
            cycles = 4;
            break;
        case ADC_A_C:
            LOG_INFO("ADC A,C");
            cpu_addcarry_to_A(gb->cpu, gb->cpu->C);
            cycles = 4;
            break;
        case ADC_A_D:
            LOG_INFO("ADC A,D");
            cpu_addcarry_to_A(gb->cpu, gb->cpu->D);
            cycles = 4;
            break;
        case ADC_A_E:
            LOG_INFO("ADC A,E");
            cpu_addcarry_to_A(gb->cpu, gb->cpu->E);
            cycles = 4;
            break;
        case ADC_A_H:
            LOG_INFO("ADC A,H");
            cpu_addcarry_to_A(gb->cpu, gb->cpu->H);
            cycles = 4;
            break;
        case ADC_A_L:
            LOG_INFO("ADC A,L");
            cpu_addcarry_to_A(gb->cpu, gb->cpu->L);
            cycles = 4;
            break;

        case ADC_A_HL:
            LOG_INFO("ADC A,(HL)");
            cpu_addcarry_to_A(gb->cpu, memory_get8(gb, cpu_get_value_HL(gb->cpu)));
            cycles = 8;
            break;

        case ADC_A_d8:
            LOG_INFO("ADC A,d8");
            cpu_addcarry_to_A(gb->cpu, gameboy_fetch_immediate8(gb));
            cycles = 8;
            break;

        // 8 bit Subtraction instructions.
        case SUB_A_A:
            LOG_INFO("SUB A,A");
            cpu_subtract_from_A(gb->cpu, gb->cpu->A);
            cycles = 4;
            break;
        case SUB_A_B:
            LOG_INFO("SUB A,B");
            cpu_subtract_from_A(gb->cpu, gb->cpu->B);
            cycles = 4;
            break;
        case SUB_A_C:
            LOG_INFO("SUB A,C");
            cpu_subtract_from_A(gb->cpu, gb->cpu->C);
            cycles = 4;
            break;
        case SUB_A_D:
            LOG_INFO("SUB A,D");
            cpu_subtract_from_A(gb->cpu, gb->cpu->D);
            cycles = 4;
            break;
        case SUB_A_E:
            LOG_INFO("SUB A,E");
            cpu_subtract_from_A(gb->cpu, gb->cpu->E);
            cycles = 4;
            break;
        case SUB_A_H:
            LOG_INFO("SUB A,H");
            cpu_subtract_from_A(gb->cpu, gb->cpu->H);
            cycles = 4;
            break;
        case SUB_A_L:
            LOG_INFO("SUB A,L");
            cpu_subtract_from_A(gb->cpu, gb->cpu->L);
            cycles = 4;
            break;

        case SUB_A_HL:
            LOG_INFO("SUB A,(HL)");
            cpu_subtract_from_A(gb->cpu, memory_get8(gb, cpu_get_value_HL(gb->cpu)));
            cycles = 8;
            break;

        case SUB_A_d8:
            LOG_INFO("SUB A,d8");
            cpu_subtract_from_A(gb->cpu, gameboy_fetch_immediate8(gb));
            cycles = 8;
            break;

        case SBC_A_A:
            LOG_INFO("SBC A,A");
            cpu_subtractcarry_from_A(gb->cpu, gb->cpu->A);
            cycles = 4;
            break;
        case SBC_A_B:
            LOG_INFO("SBC A,B");
            cpu_subtractcarry_from_A(gb->cpu, gb->cpu->B);
            cycles = 4;
            break;
        case SBC_A_C:
            LOG_INFO("SBC A,C");
            cpu_subtractcarry_from_A(gb->cpu, gb->cpu->C);
            cycles = 4;
            break;
        case SBC_A_D:
            LOG_INFO("SBC A,D");
            cpu_subtractcarry_from_A(gb->cpu, gb->cpu->D);
            cycles = 4;
            break;
        case SBC_A_E:
            LOG_INFO("SBC A,E");
            cpu_subtractcarry_from_A(gb->cpu, gb->cpu->E);
            cycles = 4;
            break;
        case SBC_A_H:
            LOG_INFO("SBC A,H");
            cpu_subtractcarry_from_A(gb->cpu, gb->cpu->H);
            cycles = 4;
            break;
        case SBC_A_L:
            LOG_INFO("SBC A,L");
            cpu_subtractcarry_from_A(gb->cpu, gb->cpu->L);
            cycles = 4;
            break;

        case SBC_A_HL:
            LOG_INFO("SBC A,(HL)");
            cpu_subtractcarry_from_A(gb->cpu, memory_get8(gb, cpu_get_value_HL(gb->cpu)));
            cycles = 8;
            break;

        case SBC_A_d8:
            LOG_INFO("SBC A,d8");
            cpu_subtractcarry_from_A(gb->cpu, gameboy_fetch_immediate8(gb));
            cycles = 8;
            break;

        // 8 bit And instructions.
        case AND_A_A:
            LOG_INFO("AND A,A");
            cpu_and_A(gb->cpu, gb->cpu->A);
            cycles = 4;
            break;
        case AND_A_B:
            LOG_INFO("AND A,B");
            cpu_and_A(gb->cpu, gb->cpu->B);
            cycles = 4;
            break;
        case AND_A_C:
            LOG_INFO("AND A,C");
            cpu_and_A(gb->cpu, gb->cpu->C);
            cycles = 4;
            break;
        case AND_A_D:
            LOG_INFO("AND A,D");
            cpu_and_A(gb->cpu, gb->cpu->D);
            cycles = 4;
            break;
        case AND_A_E:
            LOG_INFO("AND A,E");
            cpu_and_A(gb->cpu, gb->cpu->E);
            cycles = 4;
            break;
        case AND_A_H:
            LOG_INFO("AND A,H");
            cpu_and_A(gb->cpu, gb->cpu->H);
            cycles = 4;
            break;
        case AND_A_L:
            LOG_INFO("AND A,L");
            cpu_and_A(gb->cpu, gb->cpu->L);
            cycles = 4;
            break;

        case AND_A_HL:
            LOG_INFO("AND A,(HL)");
            cpu_and_A(gb->cpu, memory_get8(gb, cpu_get_value_HL(gb->cpu)));
            cycles = 8;
            break;

        case AND_A_d8:
            LOG_INFO("AND A,d8");
            cpu_and_A(gb->cpu, gameboy_fetch_immediate8(gb));
            cycles = 8;
            break;

        // 8 bit Or instructions.
        case OR_A_A:
            LOG_INFO("OR A,A");
            cpu_or_A(gb->cpu, gb->cpu->A);
            cycles = 4;
            break;
        case OR_A_B:
            LOG_INFO("OR A,B");
            cpu_or_A(gb->cpu, gb->cpu->B);
            cycles = 4;
            break;
        case OR_A_C:
            LOG_INFO("OR A,C");
            cpu_or_A(gb->cpu, gb->cpu->C);
            cycles = 4;
            break;
        case OR_A_D:
            LOG_INFO("OR A,D");
            cpu_or_A(gb->cpu, gb->cpu->D);
            cycles = 4;
            break;
        case OR_A_E:
            LOG_INFO("OR A,E");
            cpu_or_A(gb->cpu, gb->cpu->E);
            cycles = 4;
            break;
        case OR_A_H:
            LOG_INFO("OR A,H");
            cpu_or_A(gb->cpu, gb->cpu->H);
            cycles = 4;
            break;
        case OR_A_L:
            LOG_INFO("OR A,L");
            cpu_or_A(gb->cpu, gb->cpu->L);
            cycles = 4;
            break;

        case OR_A_HL:
            LOG_INFO("OR A,(HL)");
            cpu_or_A(gb->cpu, memory_get8(gb, cpu_get_value_HL(gb->cpu)));
            cycles = 8;
            break;

        case OR_A_d8:
            LOG_INFO("OR A,d8");
            cpu_or_A(gb->cpu, gameboy_fetch_immediate8(gb));
            cycles = 8;
            break;

        // 8 bit Xor instructions.
        case XOR_A_A:
            LOG_INFO("XOR A,A");
            cpu_xor_A(gb->cpu, gb->cpu->A);
            cycles = 4;
            break;
        case XOR_A_B:
            LOG_INFO("XOR A,B");
            cpu_xor_A(gb->cpu, gb->cpu->B);
            cycles = 4;
            break;
        case XOR_A_C:
            LOG_INFO("XOR A,C");
            cpu_xor_A(gb->cpu, gb->cpu->C);
            cycles = 4;
            break;
        case XOR_A_D:
            LOG_INFO("XOR A,D");
            cpu_xor_A(gb->cpu, gb->cpu->D);
            cycles = 4;
            break;
        case XOR_A_E:
            LOG_INFO("XOR A,E");
            cpu_xor_A(gb->cpu, gb->cpu->E);
            cycles = 4;
            break;
        case XOR_A_H:
            LOG_INFO("XOR A,H");
            cpu_xor_A(gb->cpu, gb->cpu->H);
            cycles = 4;
            break;
        case XOR_A_L:
            LOG_INFO("XOR A,L");
            cpu_xor_A(gb->cpu, gb->cpu->L);
            cycles = 4;
            break;

        case XOR_A_HL:
            LOG_INFO("XOR A,(HL)");
            cpu_xor_A(gb->cpu, memory_get8(gb, cpu_get_value_HL(gb->cpu)));
            cycles = 8;
            break;

        case XOR_A_d8:
            LOG_INFO("XOR A,d8");
            cpu_xor_A(gb->cpu, gameboy_fetch_immediate8(gb));
            cycles = 8;
            break;

        // 8 bit Compare instructions.
        case CP_A_A:
            LOG_INFO("CP A,A");
            cpu_compare_A(gb->cpu, gb->cpu->A);
            cycles = 4;
            break;
        case CP_A_B:
            LOG_INFO("CP A,B");
            cpu_compare_A(gb->cpu, gb->cpu->B);
            cycles = 4;
            break;
        case CP_A_C:
            LOG_INFO("CP A,C");
            cpu_compare_A(gb->cpu, gb->cpu->C);
            cycles = 4;
            break;
        case CP_A_D:
            LOG_INFO("CP A,D");
            cpu_compare_A(gb->cpu, gb->cpu->D);
            cycles = 4;
            break;
        case CP_A_E:
            LOG_INFO("CP A,E");
            cpu_compare_A(gb->cpu, gb->cpu->E);
            cycles = 4;
            break;
        case CP_A_H:
            LOG_INFO("CP A,H");
            cpu_compare_A(gb->cpu, gb->cpu->H);
            cycles = 4;
            break;
        case CP_A_L:
            LOG_INFO("CP A,L");
            cpu_compare_A(gb->cpu, gb->cpu->L);
            cycles = 4;
            break;

        case CP_A_HL:
            LOG_INFO("CP A,(HL)");
            cpu_compare_A(gb->cpu, memory_get8(gb, cpu_get_value_HL(gb->cpu)));
            cycles = 8;
            break;

        case CP_A_d8:
        {
            LOG_INFO("CP A,d8");
            LOG_DEBUG("A = $%.2x", gb->cpu->A);
            uint8_t value = gameboy_fetch_immediate8(gb);
            LOG_DEBUG("d8 = $%.2x", value);
            cpu_compare_A(gb->cpu, value);
            cycles = 8;
            break;
        }

        // 8 bit Increment instructions.
        case INC_A:
            LOG_INFO("INC A");
            cpu_increment_A(gb->cpu);
            cycles = 4;
            break;
        case INC_B:
            LOG_INFO("INC B");
            cpu_increment_B(gb->cpu);
            cycles = 4;
            break;
        case INC_C:
            LOG_INFO("INC C");
            cpu_increment_C(gb->cpu);
            cycles = 4;
            break;
        case INC_D:
            LOG_INFO("INC D");
            cpu_increment_D(gb->cpu);
            cycles = 4;
            break;
        case INC_E:
            LOG_INFO("INC E");
            cpu_increment_E(gb->cpu);
            cycles = 4;
            break;
        case INC_H:
            LOG_INFO("INC H");
            cpu_increment_H(gb->cpu);
            cycles = 4;
            break;
        case INC_L:
            LOG_INFO("INC L");
            cpu_increment_L(gb->cpu);
            cycles = 4;
            break;

        case INC_aHL:
        {
            LOG_INFO("INC (HL)");
            uint8_t current = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), cpu_increment8_value(gb->cpu, current));
            cycles = 12;
            break;
        }

        // 8 bit Decrement instructions.
        case DEC_A:
            LOG_INFO("DEC A");
            cpu_decrement_A(gb->cpu);
            cycles = 4;
            break;
        case DEC_B:
            LOG_INFO("DEC B");
            LOG_DEBUG("B was $%.2x", gb->cpu->B);
            cpu_decrement_B(gb->cpu);
            LOG_DEBUG("B = $%.2x", gb->cpu->B);
            cycles = 4;
            break;
        case DEC_C:
            LOG_INFO("DEC C");
            cpu_decrement_C(gb->cpu);
            cycles = 4;
            break;
        case DEC_D:
            LOG_INFO("DEC D");
            cpu_decrement_D(gb->cpu);
            cycles = 4;
            break;
        case DEC_E:
            LOG_INFO("DEC E");
            cpu_decrement_E(gb->cpu);
            cycles = 4;
            break;
        case DEC_H:
            LOG_INFO("DEC H");
            cpu_decrement_H(gb->cpu);
            cycles = 4;
            break;
        case DEC_L:
            LOG_INFO("DEC L");
            cpu_decrement_L(gb->cpu);
            cycles = 4;
            break;

        case DEC_aHL:
        {
            LOG_INFO("DEC (HL)");
            uint8_t current = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), cpu_decrement8_value(gb->cpu, current));
            cycles = 12;
            break;
        }



        // 16 bit ALU instructions.

        // 16 bit Add instructions.
        case ADD_HL_BC:
            LOG_INFO("ADD HL,BC");
            cpu_add16_to_HL(gb->cpu, cpu_get_value_BC(gb->cpu));
            cycles = 8;
            break;
        case ADD_HL_DE:
            LOG_INFO("ADD HL,DE");
            cpu_add16_to_HL(gb->cpu, cpu_get_value_DE(gb->cpu));
            cycles = 8;
            break;
        case ADD_HL_HL:
            LOG_INFO("ADD HL,HL");
            cpu_add16_to_HL(gb->cpu, cpu_get_value_HL(gb->cpu));
            cycles = 8;
            break;
        case ADD_HL_SP:
            LOG_INFO("ADD HL,SP");
            cpu_add16_to_HL(gb->cpu, gb->cpu->SP);
            cycles = 8;
            break;

        case ADD_SP_d8:
            LOG_INFO("ADD SP,d8");
            uint8_t value = gameboy_fetch_immediate8(gb);
            (gb->cpu->SP & 0x0F) + (value & 0x0F) > 0x0F ? cpu_flag_setH(gb->cpu) : cpu_flag_resetH(gb->cpu);
            (gb->cpu->SP & 0xFF) + value > 0xFF ? cpu_flag_setC(gb->cpu) : cpu_flag_resetC(gb->cpu);

            gb->cpu->SP += (int8_t) value;
            cpu_flag_resetZ(gb->cpu);
            cpu_flag_resetN(gb->cpu);
            cycles = 16;
            break;

        // 16 bit Increment instructions.
        case INC_BC:
            LOG_INFO("INC BC");
            cpu_increment_BC(gb->cpu);
            cycles = 8;
            break;
        case INC_DE:
            LOG_INFO("INC DE");
            cpu_increment_DE(gb->cpu);
            cycles = 8;
            break;
        case INC_HL:
            LOG_INFO("INC HL");
            cpu_increment_HL(gb->cpu);
            cycles = 8;
            break;
        case INC_SP:
            LOG_INFO("INC SP");
            gb->cpu->SP++;
            cycles = 8;
            break;

        // 16 bit Decrement instructions.
        case DEC_BC:
            LOG_INFO("DEC BC");
            cpu_decrement_BC(gb->cpu);
            cycles = 8;
            break;
        case DEC_DE:
            LOG_INFO("DEC DE");
            cpu_decrement_DE(gb->cpu);
            cycles = 8;
            break;
        case DEC_HL:
            LOG_INFO("DEC HL");
            cpu_decrement_HL(gb->cpu);
            cycles = 8;
            break;
        case DEC_SP:
            LOG_INFO("DEC SP");
            gb->cpu->SP--;
            cycles = 8;
            break;



        // Miscellaneous instructions.
        case DAA:
            LOG_INFO("DAA");
            cpu_daa(gb->cpu);
            cycles = 4;
            break;

        case CPL:
            LOG_INFO("CPL");
            gb->cpu->A = ~gb->cpu->A;

            cpu_flag_setN(gb->cpu);
            cpu_flag_setH(gb->cpu);
            cycles = 4;
            break;

        case CCF:
            LOG_INFO("CCF");
            cpu_flag_getC(gb->cpu) ? cpu_flag_resetC(gb->cpu) : cpu_flag_setC(gb->cpu);
            cpu_flag_resetN(gb->cpu);
            cpu_flag_resetH(gb->cpu);
            cycles = 4;
            break;

        case SCF:
            LOG_INFO("SCF");
            cpu_flag_setC(gb->cpu);
            cpu_flag_resetN(gb->cpu);
            cpu_flag_resetH(gb->cpu);
            cycles = 4;
            break;

        case NOP:
            LOG_INFO("NOP");
            cycles = 4;
            break;

        case HALT:
            LOG_INFO("HALT");
            // TODO(mct)
            cycles = 4;
            break;

        case STOP:
            LOG_INFO("STOP");
            gameboy_fetch_immediate8(gb);   // This should be the value 0x00.
            // TODO(mct)
            cycles = 4;
            break;

        case DI:
            LOG_INFO("DI");
            // Warning: Timing may be wrong.
            gb->int_master_enable = 0;
            cycles = 4;
            break;
        case EI:
            LOG_INFO("EI");
            // Warning: Timing may be wrong.
            gb->int_master_enable = 1;
            cycles = 4;
            break;



        // Rotate instructions.
        case RLCA:
            LOG_INFO("RLCA");
            gb->cpu->A = cpu_rlc_value(gb->cpu, gb->cpu->A);
            cpu_flag_resetZ(gb->cpu);
            cycles = 4;
            break;
        case RLA:
            LOG_INFO("RLA");
            gb->cpu->A = cpu_rl_value(gb->cpu, gb->cpu->A);
            cpu_flag_resetZ(gb->cpu);
            cycles = 4;
            break;
        case RRCA:
            LOG_INFO("RRCA");
            gb->cpu->A = cpu_rrc_value(gb->cpu, gb->cpu->A);
            cpu_flag_resetZ(gb->cpu);
            cycles = 4;
            break;
        case RRA:
            LOG_INFO("RRA");
            gb->cpu->A = cpu_rr_value(gb->cpu, gb->cpu->A);
            cpu_flag_resetZ(gb->cpu);
            cycles = 4;
            break;



        // Jump instructions.
        case JP_a16:
            LOG_INFO("JP a16");
            gb->cpu->PC = gameboy_fetch_immediate16(gb);
            cycles = 12;
            break;

        case JP_NZ_a16:
        {
            LOG_INFO("JP NZ,a16");
            uint16_t address = gameboy_fetch_immediate16(gb);
            if (!cpu_flag_getZ(gb->cpu)) gb->cpu->PC = address;
            cycles = 12;
            break;
        }
        case JP_Z_a16:
        {
            LOG_INFO("JP Z,a16");
            uint16_t address = gameboy_fetch_immediate16(gb);
            if (cpu_flag_getZ(gb->cpu)) gb->cpu->PC = address;
            cycles = 12;
            break;
        }
        case JP_NC_a16:
        {
            LOG_INFO("JP NC,a16");
            uint16_t address = gameboy_fetch_immediate16(gb);
            if (!cpu_flag_getC(gb->cpu)) gb->cpu->PC = address;
            cycles = 12;
            break;
        }
        case JP_C_a16:
        {
            LOG_INFO("JP C,a16");
            uint16_t address = gameboy_fetch_immediate16(gb);
            if (cpu_flag_getC(gb->cpu)) gb->cpu->PC = address;
            cycles = 12;
            break;
        }

        case JP_HL:
            LOG_INFO("JP (HL)");
            gb->cpu->PC = cpu_get_value_HL(gb->cpu);
            cycles = 4;
            break;

        case JR_d8:
            LOG_INFO("JR d8");
            gb->cpu->PC += ((int8_t) gameboy_fetch_immediate8(gb));
            cycles = 8;
            break;

        case JR_NZ_a16:
        {
            LOG_INFO("JR NZ,a16");
            int8_t offset = (int8_t) gameboy_fetch_immediate8(gb);
            LOG_DEBUG("offset = %d", offset);
            if (!cpu_flag_getZ(gb->cpu)) gb->cpu->PC += offset;
            LOG_DEBUG("Jumping to 0x%.4x", gb->cpu->PC);
            cycles = 8;
            break;
        }
        case JR_Z_a16:
        {
            LOG_INFO("JR Z,a16");
            int8_t offset = (int8_t) gameboy_fetch_immediate8(gb);
            if (cpu_flag_getZ(gb->cpu)) gb->cpu->PC += offset;
            cycles = 8;
            break;
        }
        case JR_NC_a16:
        {
            LOG_INFO("JR NC,a16");
            int8_t offset = (int8_t) gameboy_fetch_immediate8(gb);
            if (!cpu_flag_getC(gb->cpu)) gb->cpu->PC += offset;
            cycles = 8;
            break;
        }
        case JR_C_a16:
        {
            LOG_INFO("JR C,a16");
            int8_t offset = (int8_t) gameboy_fetch_immediate8(gb);
            if (cpu_flag_getC(gb->cpu)) gb->cpu->PC += offset;
            cycles = 8;
            break;
        }



        // Calls
        case CALL_a16:
        {
            LOG_INFO("CALL a16");
            uint16_t address = gameboy_fetch_immediate16(gb);
            gameboy_push16(gb, gb->cpu->PC);
            gb->cpu->PC = address;
            cycles = 12;
            break;
        }

        case CALL_NZ_a16:
        {
            LOG_INFO("CALL NZ,a16");
            uint16_t address = gameboy_fetch_immediate16(gb);
            if (!cpu_flag_getZ(gb->cpu)) {
                gameboy_push16(gb, gb->cpu->PC);
                gb->cpu->PC = address;
            }
            cycles = 12;
            break;
        }
        case CALL_Z_a16:
        {
            LOG_INFO("CALL Z,a16");
            uint16_t address = gameboy_fetch_immediate16(gb);
            if (cpu_flag_getZ(gb->cpu)) {
                gameboy_push16(gb, gb->cpu->PC);
                gb->cpu->PC = address;
            }
            cycles = 12;
            break;
        }
        case CALL_NC_a16:
        {
            LOG_INFO("CALL NC,a16");
            uint16_t address = gameboy_fetch_immediate16(gb);
            if (!cpu_flag_getC(gb->cpu)) {
                gameboy_push16(gb, gb->cpu->PC);
                gb->cpu->PC = address;
            }
            cycles = 12;
            break;
        }
        case CALL_C_a16:
        {
            LOG_INFO("CALL C,a16");
            uint16_t address = gameboy_fetch_immediate16(gb);
            if (cpu_flag_getC(gb->cpu)) {
                gameboy_push16(gb, gb->cpu->PC);
                gb->cpu->PC = address;
            }
            cycles = 12;
            break;
        }



        // Returns
        case RET:
            LOG_INFO("RET");
            gb->cpu->PC = gameboy_pop16(gb);
            cycles = 8;
            break;

        case RET_NZ:
            LOG_INFO("RET NZ");
            if (!cpu_flag_getZ(gb->cpu)) gb->cpu->PC = gameboy_pop16(gb);
            cycles = 8;
            break;
        case RET_Z:
            LOG_INFO("RET Z");
            if (cpu_flag_getZ(gb->cpu)) gb->cpu->PC = gameboy_pop16(gb);
            cycles = 8;
            break;
        case RET_NC:
            LOG_INFO("RET NC");
            if (!cpu_flag_getC(gb->cpu)) gb->cpu->PC = gameboy_pop16(gb);
            cycles = 8;
            break;
        case RET_C:
            LOG_INFO("RET C");
            if (cpu_flag_getC(gb->cpu)) gb->cpu->PC = gameboy_pop16(gb);
            cycles = 8;
            break;

        case RETI:
            LOG_INFO("RETI");
            gb->cpu->PC = gameboy_pop16(gb);

            // Warning: Timing may be wrong.
            gb->int_master_enable = 1;
            cycles = 8;
            break;



        // Restarts
        // WARNING: Not sure if we are pushing the correct address.
        case RST_00H:
            LOG_INFO("RST 00H");
            gameboy_push16(gb, gb->cpu->PC);
            gb->cpu->PC = 0x00;
            cycles = 32;
            break;
        case RST_08H:
            LOG_INFO("RST 08H");
            gameboy_push16(gb, gb->cpu->PC);
            gb->cpu->PC = 0x08;
            cycles = 32;
            break;
        case RST_10H:
            LOG_INFO("RST 10H");
            gameboy_push16(gb, gb->cpu->PC);
            gb->cpu->PC = 0x10;
            cycles = 32;
            break;
        case RST_18H:
            LOG_INFO("RST 18H");
            gameboy_push16(gb, gb->cpu->PC);
            gb->cpu->PC = 0x18;
            cycles = 32;
            break;
        case RST_20H:
            LOG_INFO("RST 20H");
            gameboy_push16(gb, gb->cpu->PC);
            gb->cpu->PC = 0x20;
            cycles = 32;
            break;
        case RST_28H:
            LOG_INFO("RST 28H");
            gameboy_push16(gb, gb->cpu->PC);
            gb->cpu->PC = 0x28;
            cycles = 32;
            break;
        case RST_30H:
            LOG_INFO("RST 30H");
            gameboy_push16(gb, gb->cpu->PC);
            gb->cpu->PC = 0x30;
            cycles = 32;
            break;
        case RST_38H:
            LOG_INFO("RST 38H");
            gameboy_push16(gb, gb->cpu->PC);
            gb->cpu->PC = 0x38;
            cycles = 32;
            break;



        // CB prefix instructions.
        case CB_PREFIX:
        {
            uint8_t base = gameboy_fetch_immediate8(gb);
            cycles = gameboy_execute_cb_prefix_instruction(gb, base);
            break;
        }


        default:
            LOG_ERROR("Instruction %x not found.", instruction);
            exit(1);
    }
    return cycles;
}

/** Executes a instruction with CB prefix.
 *  @param gb Gameboy to execute the instruction on.
 *  @param base The 'base' of the instruction (byte after CB prefix).
 *  
 * @return The number of cpu cycles the instruction took to execute.
*/
uint8_t gameboy_execute_cb_prefix_instruction(Gameboy* gb, uint8_t base) {
    uint8_t cycles = 0;
    switch (base) {
        // Swap instructions.
        case SWAP_A:
            LOG_INFO("SWAP A");
            gb->cpu->A = cpu_swap_value(gb->cpu, gb->cpu->A);
            cycles = 8;
            break;
        case SWAP_B:
            LOG_INFO("SWAP B");
            gb->cpu->B = cpu_swap_value(gb->cpu, gb->cpu->B);
            cycles = 8;
            break;
        case SWAP_C:
            LOG_INFO("SWAP C");
            gb->cpu->C = cpu_swap_value(gb->cpu, gb->cpu->C);
            cycles = 8;
            break;
        case SWAP_D:
            LOG_INFO("SWAP D");
            gb->cpu->D = cpu_swap_value(gb->cpu, gb->cpu->D);
            cycles = 8;
            break;
        case SWAP_E:
            LOG_INFO("SWAP E");
            gb->cpu->E = cpu_swap_value(gb->cpu, gb->cpu->E);
            cycles = 8;
            break;
        case SWAP_H:
            LOG_INFO("SWAP H");
            gb->cpu->H = cpu_swap_value(gb->cpu, gb->cpu->H);
            cycles = 8;
            break;
        case SWAP_L:
            LOG_INFO("SWAP L");
            gb->cpu->L = cpu_swap_value(gb->cpu, gb->cpu->L);
            cycles = 8;
            break;

        case SWAP_HL:
        {
            LOG_INFO("SWAP (HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), cpu_swap_value(gb->cpu, value));
            cycles = 16;
            break;
        }

        // Rotate instructions.
        case RLC_A:
            LOG_INFO("RLC A");
            gb->cpu->A = cpu_rlc_value(gb->cpu, gb->cpu->A);
            cycles = 8;
            break;
        case RLC_B:
            LOG_INFO("RLC B");
            gb->cpu->B = cpu_rlc_value(gb->cpu, gb->cpu->B);
            cycles = 8;
            break;
        case RLC_C:
            LOG_INFO("RLC C");
            gb->cpu->C = cpu_rlc_value(gb->cpu, gb->cpu->C);
            cycles = 8;
            break;
        case RLC_D:
            LOG_INFO("RLC D");
            gb->cpu->D = cpu_rlc_value(gb->cpu, gb->cpu->D);
            cycles = 8;
            break;
        case RLC_E:
            LOG_INFO("RLC E");
            gb->cpu->E = cpu_rlc_value(gb->cpu, gb->cpu->E);
            cycles = 8;
            break;
        case RLC_H:
            LOG_INFO("RLC H");
            gb->cpu->H = cpu_rlc_value(gb->cpu, gb->cpu->H);
            cycles = 8;
            break;
        case RLC_L:
            LOG_INFO("RLC L");
            gb->cpu->L = cpu_rlc_value(gb->cpu, gb->cpu->L);
            cycles = 8;
            break;

        case RLC_HL:
        {
            LOG_INFO("RLC (HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), cpu_rlc_value(gb->cpu, value));
            cycles = 16;
            break;
        }


        case RL_A:
            LOG_INFO("RL A");
            gb->cpu->A = cpu_rl_value(gb->cpu, gb->cpu->A);
            cycles = 8;
            break;
        case RL_B:
            LOG_INFO("RL B");
            gb->cpu->B = cpu_rl_value(gb->cpu, gb->cpu->B);
            cycles = 8;
            break;
        case RL_C:
            LOG_INFO("RL C");
            gb->cpu->C = cpu_rl_value(gb->cpu, gb->cpu->C);
            cycles = 8;
            break;
        case RL_D:
            LOG_INFO("RL D");
            gb->cpu->D = cpu_rl_value(gb->cpu, gb->cpu->D);
            cycles = 8;
            break;
        case RL_E:
            LOG_INFO("RL E");
            gb->cpu->E = cpu_rl_value(gb->cpu, gb->cpu->E);
            cycles = 8;
            break;
        case RL_H:
            LOG_INFO("RL H");
            gb->cpu->H = cpu_rl_value(gb->cpu, gb->cpu->H);
            cycles = 8;
            break;
        case RL_L:
            LOG_INFO("RL L");
            gb->cpu->L = cpu_rl_value(gb->cpu, gb->cpu->L);
            cycles = 8;
            break;

        case RL_HL:
        {
            LOG_INFO("RL (HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), cpu_rl_value(gb->cpu, value));
            cycles = 16;
            break;
        }


        case RRC_A:
            LOG_INFO("RRC A");
            gb->cpu->A = cpu_rrc_value(gb->cpu, gb->cpu->A);
            cycles = 8;
            break;
        case RRC_B:
            LOG_INFO("RRC B");
            gb->cpu->B = cpu_rrc_value(gb->cpu, gb->cpu->B);
            cycles = 8;
            break;
        case RRC_C:
            LOG_INFO("RRC C");
            gb->cpu->C = cpu_rrc_value(gb->cpu, gb->cpu->C);
            cycles = 8;
            break;
        case RRC_D:
            LOG_INFO("RRC D");
            gb->cpu->D = cpu_rrc_value(gb->cpu, gb->cpu->D);
            cycles = 8;
            break;
        case RRC_E:
            LOG_INFO("RRC E");
            gb->cpu->E = cpu_rrc_value(gb->cpu, gb->cpu->E);
            cycles = 8;
            break;
        case RRC_H:
            LOG_INFO("RRC H");
            gb->cpu->H = cpu_rrc_value(gb->cpu, gb->cpu->H);
            cycles = 8;
            break;
        case RRC_L:
            LOG_INFO("RRC L");
            gb->cpu->L = cpu_rrc_value(gb->cpu, gb->cpu->L);
            cycles = 8;
            break;

        case RRC_HL:
        {
            LOG_INFO("RRC (HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), cpu_rrc_value(gb->cpu, value));
            cycles = 16;
            break;
        }


        case RR_A:
            LOG_INFO("RR A");
            gb->cpu->A = cpu_rr_value(gb->cpu, gb->cpu->A);
            cycles = 8;
            break;
        case RR_B:
            LOG_INFO("RR B");
            gb->cpu->B = cpu_rr_value(gb->cpu, gb->cpu->B);
            cycles = 8;
            break;
        case RR_C:
            LOG_INFO("RR C");
            gb->cpu->C = cpu_rr_value(gb->cpu, gb->cpu->C);
            cycles = 8;
            break;
        case RR_D:
            LOG_INFO("RR D");
            gb->cpu->D = cpu_rr_value(gb->cpu, gb->cpu->D);
            cycles = 8;
            break;
        case RR_E:
            LOG_INFO("RR E");
            gb->cpu->E = cpu_rr_value(gb->cpu, gb->cpu->E);
            cycles = 8;
            break;
        case RR_H:
            LOG_INFO("RR H");
            gb->cpu->H = cpu_rr_value(gb->cpu, gb->cpu->H);
            cycles = 8;
            break;
        case RR_L:
            LOG_INFO("RR L");
            gb->cpu->L = cpu_rr_value(gb->cpu, gb->cpu->L);
            cycles = 8;
            break;

        case RR_HL:
        {
            LOG_INFO("RR (HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), cpu_rr_value(gb->cpu, value));
            cycles = 16;
            break;
        }


        case SLA_A:
            LOG_INFO("SLA A");
            gb->cpu->A = cpu_sla_value(gb->cpu, gb->cpu->A);
            cycles = 8;
            break;
        case SLA_B:
            LOG_INFO("SLA B");
            gb->cpu->B = cpu_sla_value(gb->cpu, gb->cpu->B);
            cycles = 8;
            break;
        case SLA_C:
            LOG_INFO("SLA C");
            gb->cpu->C = cpu_sla_value(gb->cpu, gb->cpu->C);
            cycles = 8;
            break;
        case SLA_D:
            LOG_INFO("SLA D");
            gb->cpu->D = cpu_sla_value(gb->cpu, gb->cpu->D);
            cycles = 8;
            break;
        case SLA_E:
            LOG_INFO("SLA E");
            gb->cpu->E = cpu_sla_value(gb->cpu, gb->cpu->E);
            cycles = 8;
            break;
        case SLA_H:
            LOG_INFO("SLA H");
            gb->cpu->H = cpu_sla_value(gb->cpu, gb->cpu->H);
            cycles = 8;
            break;
        case SLA_L:
            LOG_INFO("SLA L");
            gb->cpu->L = cpu_sla_value(gb->cpu, gb->cpu->L);
            cycles = 8;
            break;

        case SLA_HL:
        {
            LOG_INFO("SLA (HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), cpu_sla_value(gb->cpu, value));
            cycles = 16;
            break;
        }


        case SRA_A:
            LOG_INFO("SRA A");
            gb->cpu->A = cpu_sra_value(gb->cpu, gb->cpu->A);
            cycles = 8;
            break;
        case SRA_B:
            LOG_INFO("SRB B");
            gb->cpu->B = cpu_sra_value(gb->cpu, gb->cpu->B);
            cycles = 8;
            break;
        case SRA_C:
            LOG_INFO("SRC C");
            gb->cpu->C = cpu_sra_value(gb->cpu, gb->cpu->C);
            cycles = 8;
            break;
        case SRA_D:
            LOG_INFO("SRD D");
            gb->cpu->D = cpu_sra_value(gb->cpu, gb->cpu->D);
            cycles = 8;
            break;
        case SRA_E:
            LOG_INFO("SRE E");
            gb->cpu->E = cpu_sra_value(gb->cpu, gb->cpu->E);
            cycles = 8;
            break;
        case SRA_H:
            LOG_INFO("SRH H");
            gb->cpu->H = cpu_sra_value(gb->cpu, gb->cpu->H);
            cycles = 8;
            break;
        case SRA_L:
            LOG_INFO("SRL L");
            gb->cpu->L = cpu_sra_value(gb->cpu, gb->cpu->L);
            cycles = 8;
            break;

        case SRA_HL:
        {
            LOG_INFO("SRA (HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), cpu_sra_value(gb->cpu, value));
            cycles = 16;
            break;
        }


        case SRL_A:
            LOG_INFO("SRL A");
            gb->cpu->A = cpu_srl_value(gb->cpu, gb->cpu->A);
            cycles = 8;
            break;
        case SRL_B:
            LOG_INFO("SRL B");
            gb->cpu->B = cpu_srl_value(gb->cpu, gb->cpu->B);
            cycles = 8;
            break;
        case SRL_C:
            LOG_INFO("SRL C");
            gb->cpu->C = cpu_srl_value(gb->cpu, gb->cpu->C);
            cycles = 8;
            break;
        case SRL_D:
            LOG_INFO("SRL D");
            gb->cpu->D = cpu_srl_value(gb->cpu, gb->cpu->D);
            cycles = 8;
            break;
        case SRL_E:
            LOG_INFO("SRL E");
            gb->cpu->E = cpu_srl_value(gb->cpu, gb->cpu->E);
            cycles = 8;
            break;
        case SRL_H:
            LOG_INFO("SRL H");
            gb->cpu->H = cpu_srl_value(gb->cpu, gb->cpu->H);
            cycles = 8;
            break;
        case SRL_L:
            LOG_INFO("SRL L");
            gb->cpu->L = cpu_srl_value(gb->cpu, gb->cpu->L);
            cycles = 8;
            break;

        case SRL_HL:
        {
            LOG_INFO("SRL (HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), cpu_srl_value(gb->cpu, value));
            cycles = 16;
            break;
        }



        // Bit opcodes.
        case BIT_0_B:
            LOG_INFO("BIT 0,B");
            cpu_test_bit_value(gb->cpu, gb->cpu->B, 0);
            cycles = 8;
            break;
        case BIT_1_B:
            LOG_INFO("BIT 1,B");
            cpu_test_bit_value(gb->cpu, gb->cpu->B, 1);
            cycles = 8;
            break;
        case BIT_2_B:
            LOG_INFO("BIT 2,B");
            cpu_test_bit_value(gb->cpu, gb->cpu->B, 2);
            cycles = 8;
            break;
        case BIT_3_B:
            LOG_INFO("BIT 3,B");
            cpu_test_bit_value(gb->cpu, gb->cpu->B, 3);
            cycles = 8;
            break;
        case BIT_4_B:
            LOG_INFO("BIT 4,B");
            cpu_test_bit_value(gb->cpu, gb->cpu->B, 4);
            cycles = 8;
            break;
        case BIT_5_B:
            LOG_INFO("BIT 5,B");
            cpu_test_bit_value(gb->cpu, gb->cpu->B, 5);
            cycles = 8;
            break;
        case BIT_6_B:
            LOG_INFO("BIT 6,B");
            cpu_test_bit_value(gb->cpu, gb->cpu->B, 6);
            cycles = 8;
            break;
        case BIT_7_B:
            LOG_INFO("BIT 7,B");
            cpu_test_bit_value(gb->cpu, gb->cpu->B, 7);
            cycles = 8;
            break;

        case BIT_0_C:
            LOG_INFO("BIT 0,C");
            cpu_test_bit_value(gb->cpu, gb->cpu->C, 0);
            cycles = 8;
            break;
        case BIT_1_C:
            LOG_INFO("BIT 1,C");
            cpu_test_bit_value(gb->cpu, gb->cpu->C, 1);
            cycles = 8;
            break;
        case BIT_2_C:
            LOG_INFO("BIT 2,C");
            cpu_test_bit_value(gb->cpu, gb->cpu->C, 2);
            cycles = 8;
            break;
        case BIT_3_C:
            LOG_INFO("BIT 3,C");
            cpu_test_bit_value(gb->cpu, gb->cpu->C, 3);
            cycles = 8;
            break;
        case BIT_4_C:
            LOG_INFO("BIT 4,C");
            cpu_test_bit_value(gb->cpu, gb->cpu->C, 4);
            cycles = 8;
            break;
        case BIT_5_C:
            LOG_INFO("BIT 5,C");
            cpu_test_bit_value(gb->cpu, gb->cpu->C, 5);
            cycles = 8;
            break;
        case BIT_6_C:
            LOG_INFO("BIT 6,C");
            cpu_test_bit_value(gb->cpu, gb->cpu->C, 6);
            cycles = 8;
            break;
        case BIT_7_C:
            LOG_INFO("BIT 7,C");
            cpu_test_bit_value(gb->cpu, gb->cpu->C, 7);
            cycles = 8;
            break;

        case BIT_0_D:
            LOG_INFO("BIT 0,D");
            cpu_test_bit_value(gb->cpu, gb->cpu->D, 0);
            cycles = 8;
            break;
        case BIT_1_D:
            LOG_INFO("BIT 1,D");
            cpu_test_bit_value(gb->cpu, gb->cpu->D, 1);
            cycles = 8;
            break;
        case BIT_2_D:
            LOG_INFO("BIT 2,D");
            cpu_test_bit_value(gb->cpu, gb->cpu->D, 2);
            cycles = 8;
            break;
        case BIT_3_D:
            LOG_INFO("BIT 3,D");
            cpu_test_bit_value(gb->cpu, gb->cpu->D, 3);
            cycles = 8;
            break;
        case BIT_4_D:
            LOG_INFO("BIT 4,D");
            cpu_test_bit_value(gb->cpu, gb->cpu->D, 4);
            cycles = 8;
            break;
        case BIT_5_D:
            LOG_INFO("BIT 5,D");
            cpu_test_bit_value(gb->cpu, gb->cpu->D, 5);
            cycles = 8;
            break;
        case BIT_6_D:
            LOG_INFO("BIT 6,D");
            cpu_test_bit_value(gb->cpu, gb->cpu->D, 6);
            cycles = 8;
            break;
        case BIT_7_D:
            LOG_INFO("BIT 7,D");
            cpu_test_bit_value(gb->cpu, gb->cpu->D, 7);
            cycles = 8;
            break;

        case BIT_0_E:
            LOG_INFO("BIT 0,E");
            cpu_test_bit_value(gb->cpu, gb->cpu->E, 0);
            cycles = 8;
            break;
        case BIT_1_E:
            LOG_INFO("BIT 1,E");
            cpu_test_bit_value(gb->cpu, gb->cpu->E, 1);
            cycles = 8;
            break;
        case BIT_2_E:
            LOG_INFO("BIT 2,E");
            cpu_test_bit_value(gb->cpu, gb->cpu->E, 2);
            cycles = 8;
            break;
        case BIT_3_E:
            LOG_INFO("BIT 3,E");
            cpu_test_bit_value(gb->cpu, gb->cpu->E, 3);
            cycles = 8;
            break;
        case BIT_4_E:
            LOG_INFO("BIT 4,E");
            cpu_test_bit_value(gb->cpu, gb->cpu->E, 4);
            cycles = 8;
            break;
        case BIT_5_E:
            LOG_INFO("BIT 5,E");
            cpu_test_bit_value(gb->cpu, gb->cpu->E, 5);
            cycles = 8;
            break;
        case BIT_6_E:
            LOG_INFO("BIT 6,E");
            cpu_test_bit_value(gb->cpu, gb->cpu->E, 6);
            cycles = 8;
            break;
        case BIT_7_E:
            LOG_INFO("BIT 7,E");
            cpu_test_bit_value(gb->cpu, gb->cpu->E, 7);
            cycles = 8;
            break;

        case BIT_0_H:
            LOG_INFO("BIT 0,H");
            cpu_test_bit_value(gb->cpu, gb->cpu->H, 0);
            cycles = 8;
            break;
        case BIT_1_H:
            LOG_INFO("BIT 1,H");
            cpu_test_bit_value(gb->cpu, gb->cpu->H, 1);
            cycles = 8;
            break;
        case BIT_2_H:
            LOG_INFO("BIT 2,H");
            cpu_test_bit_value(gb->cpu, gb->cpu->H, 2);
            cycles = 8;
            break;
        case BIT_3_H:
            LOG_INFO("BIT 3,H");
            cpu_test_bit_value(gb->cpu, gb->cpu->H, 3);
            cycles = 8;
            break;
        case BIT_4_H:
            LOG_INFO("BIT 4,H");
            cpu_test_bit_value(gb->cpu, gb->cpu->H, 4);
            cycles = 8;
            break;
        case BIT_5_H:
            LOG_INFO("BIT 5,H");
            cpu_test_bit_value(gb->cpu, gb->cpu->H, 5);
            cycles = 8;
            break;
        case BIT_6_H:
            LOG_INFO("BIT 6,H");
            cpu_test_bit_value(gb->cpu, gb->cpu->H, 6);
            cycles = 8;
            break;
        case BIT_7_H:
            LOG_INFO("BIT 7,H");
            cpu_test_bit_value(gb->cpu, gb->cpu->H, 7);
            cycles = 8;
            break;

        case BIT_0_L:
            LOG_INFO("BIT 0,L");
            cpu_test_bit_value(gb->cpu, gb->cpu->L, 0);
            cycles = 8;
            break;
        case BIT_1_L:
            LOG_INFO("BIT 1,L");
            cpu_test_bit_value(gb->cpu, gb->cpu->L, 1);
            cycles = 8;
            break;
        case BIT_2_L:
            LOG_INFO("BIT 2,L");
            cpu_test_bit_value(gb->cpu, gb->cpu->L, 2);
            cycles = 8;
            break;
        case BIT_3_L:
            LOG_INFO("BIT 3,L");
            cpu_test_bit_value(gb->cpu, gb->cpu->L, 3);
            cycles = 8;
            break;
        case BIT_4_L:
            LOG_INFO("BIT 4,L");
            cpu_test_bit_value(gb->cpu, gb->cpu->L, 4);
            cycles = 8;
            break;
        case BIT_5_L:
            LOG_INFO("BIT 5,L");
            cpu_test_bit_value(gb->cpu, gb->cpu->L, 5);
            cycles = 8;
            break;
        case BIT_6_L:
            LOG_INFO("BIT 6,L");
            cpu_test_bit_value(gb->cpu, gb->cpu->L, 6);
            cycles = 8;
            break;
        case BIT_7_L:
            LOG_INFO("BIT 7,L");
            cpu_test_bit_value(gb->cpu, gb->cpu->L, 7);
            cycles = 8;
            break;

        case BIT_0_HL:
        {
            LOG_INFO("BIT 0,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cpu_test_bit_value(gb->cpu, value, 0);
            cycles = 16;
            break;
        }
        case BIT_1_HL:
        {
            LOG_INFO("BIT 1,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cpu_test_bit_value(gb->cpu, value, 1);
            cycles = 16;
            break;
        }
        case BIT_2_HL:
        {
            LOG_INFO("BIT 2,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cpu_test_bit_value(gb->cpu, value, 2);
            cycles = 16;
            break;
        }
        case BIT_3_HL:
        {
            LOG_INFO("BIT 3,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cpu_test_bit_value(gb->cpu, value, 3);
            cycles = 16;
            break;
        }
        case BIT_4_HL:
        {
            LOG_INFO("BIT 4,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cpu_test_bit_value(gb->cpu, value, 4);
            cycles = 16;
            break;
        }
        case BIT_5_HL:
        {
            LOG_INFO("BIT 5,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cpu_test_bit_value(gb->cpu, value, 5);
            cycles = 16;
            break;
        }
        case BIT_6_HL:
        {
            LOG_INFO("BIT 6,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cpu_test_bit_value(gb->cpu, value, 6);
            cycles = 16;
            break;
        }
        case BIT_7_HL:
        {
            LOG_INFO("BIT 7,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            cpu_test_bit_value(gb->cpu, value, 7);
            cycles = 16;
            break;
        }

        case BIT_0_A:
            LOG_INFO("BIT 0,A");
            cpu_test_bit_value(gb->cpu, gb->cpu->A, 0);
            cycles = 8;
            break;
        case BIT_1_A:
            LOG_INFO("BIT 1,A");
            cpu_test_bit_value(gb->cpu, gb->cpu->A, 1);
            cycles = 8;
            break;
        case BIT_2_A:
            LOG_INFO("BIT 2,A");
            cpu_test_bit_value(gb->cpu, gb->cpu->A, 2);
            cycles = 8;
            break;
        case BIT_3_A:
            LOG_INFO("BIT 3,A");
            cpu_test_bit_value(gb->cpu, gb->cpu->A, 3);
            cycles = 8;
            break;
        case BIT_4_A:
            LOG_INFO("BIT 4,A");
            cpu_test_bit_value(gb->cpu, gb->cpu->A, 4);
            cycles = 8;
            break;
        case BIT_5_A:
            LOG_INFO("BIT 5,A");
            cpu_test_bit_value(gb->cpu, gb->cpu->A, 5);
            cycles = 8;
            break;
        case BIT_6_A:
            LOG_INFO("BIT 6,A");
            cpu_test_bit_value(gb->cpu, gb->cpu->A, 6);
            cycles = 8;
            break;
        case BIT_7_A:
            LOG_INFO("BIT 7,A");
            cpu_test_bit_value(gb->cpu, gb->cpu->A, 7);
            cycles = 8;
            break;



        case SET_0_B:
            LOG_INFO("SET 0,B");
            gb->cpu->B = gb->cpu->B | (1 << 0);
            cycles = 8;
            break;
        case SET_1_B:
            LOG_INFO("SET 1,B");
            gb->cpu->B = gb->cpu->B | (1 << 1);
            cycles = 8;
            break;
        case SET_2_B:
            LOG_INFO("SET 2,B");
            gb->cpu->B = gb->cpu->B | (1 << 2);
            cycles = 8;
            break;
        case SET_3_B:
            LOG_INFO("SET 3,B");
            gb->cpu->B = gb->cpu->B | (1 << 3);
            cycles = 8;
            break;
        case SET_4_B:
            LOG_INFO("SET 4,B");
            gb->cpu->B = gb->cpu->B | (1 << 4);
            cycles = 8;
            break;
        case SET_5_B:
            LOG_INFO("SET 5,B");
            gb->cpu->B = gb->cpu->B | (1 << 5);
            cycles = 8;
            break;
        case SET_6_B:
            LOG_INFO("SET 6,B");
            gb->cpu->B = gb->cpu->B | (1 << 6);
            cycles = 8;
            break;
        case SET_7_B:
            LOG_INFO("SET 7,B");
            gb->cpu->B = gb->cpu->B | (1 << 7);
            cycles = 8;
            break;

        case SET_0_C:
            LOG_INFO("SET 0,C");
            gb->cpu->C = gb->cpu->C | (1 << 0);
            cycles = 8;
            break;
        case SET_1_C:
            LOG_INFO("SET 1,C");
            gb->cpu->C = gb->cpu->C | (1 << 1);
            cycles = 8;
            break;
        case SET_2_C:
            LOG_INFO("SET 2,C");
            gb->cpu->C = gb->cpu->C | (1 << 2);
            cycles = 8;
            break;
        case SET_3_C:
            LOG_INFO("SET 3,C");
            gb->cpu->C = gb->cpu->C | (1 << 3);
            cycles = 8;
            break;
        case SET_4_C:
            LOG_INFO("SET 4,C");
            gb->cpu->C = gb->cpu->C | (1 << 4);
            cycles = 8;
            break;
        case SET_5_C:
            LOG_INFO("SET 5,C");
            gb->cpu->C = gb->cpu->C | (1 << 5);
            cycles = 8;
            break;
        case SET_6_C:
            LOG_INFO("SET 6,C");
            gb->cpu->C = gb->cpu->C | (1 << 6);
            cycles = 8;
            break;
        case SET_7_C:
            LOG_INFO("SET 7,C");
            gb->cpu->C = gb->cpu->C | (1 << 7);
            cycles = 8;
            break;

        case SET_0_D:
            LOG_INFO("SET 0,D");
            gb->cpu->D = gb->cpu->D | (1 << 0);
            cycles = 8;
            break;
        case SET_1_D:
            LOG_INFO("SET 1,D");
            gb->cpu->D = gb->cpu->D | (1 << 1);
            cycles = 8;
            break;
        case SET_2_D:
            LOG_INFO("SET 2,D");
            gb->cpu->D = gb->cpu->D | (1 << 2);
            cycles = 8;
            break;
        case SET_3_D:
            LOG_INFO("SET 3,D");
            gb->cpu->D = gb->cpu->D | (1 << 3);
            cycles = 8;
            break;
        case SET_4_D:
            LOG_INFO("SET 4,D");
            gb->cpu->D = gb->cpu->D | (1 << 4);
            cycles = 8;
            break;
        case SET_5_D:
            LOG_INFO("SET 5,D");
            gb->cpu->D = gb->cpu->D | (1 << 5);
            cycles = 8;
            break;
        case SET_6_D:
            LOG_INFO("SET 6,D");
            gb->cpu->D = gb->cpu->D | (1 << 6);
            cycles = 8;
            break;
        case SET_7_D:
            LOG_INFO("SET 7,D");
            gb->cpu->D = gb->cpu->D | (1 << 7);
            cycles = 8;
            break;

        case SET_0_E:
            LOG_INFO("SET 0,E");
            gb->cpu->E = gb->cpu->E | (1 << 0);
            cycles = 8;
            break;
        case SET_1_E:
            LOG_INFO("SET 1,E");
            gb->cpu->E = gb->cpu->E | (1 << 1);
            cycles = 8;
            break;
        case SET_2_E:
            LOG_INFO("SET 2,E");
            gb->cpu->E = gb->cpu->E | (1 << 2);
            cycles = 8;
            break;
        case SET_3_E:
            LOG_INFO("SET 3,E");
            gb->cpu->E = gb->cpu->E | (1 << 3);
            cycles = 8;
            break;
        case SET_4_E:
            LOG_INFO("SET 4,E");
            gb->cpu->E = gb->cpu->E | (1 << 4);
            cycles = 8;
            break;
        case SET_5_E:
            LOG_INFO("SET 5,E");
            gb->cpu->E = gb->cpu->E | (1 << 5);
            cycles = 8;
            break;
        case SET_6_E:
            LOG_INFO("SET 6,E");
            gb->cpu->E = gb->cpu->E | (1 << 6);
            cycles = 8;
            break;
        case SET_7_E:
            LOG_INFO("SET 7,E");
            gb->cpu->E = gb->cpu->E | (1 << 7);
            cycles = 8;
            break;

        case SET_0_H:
            LOG_INFO("SET 0,H");
            gb->cpu->H = gb->cpu->H | (1 << 0);
            cycles = 8;
            break;
        case SET_1_H:
            LOG_INFO("SET 1,H");
            gb->cpu->H = gb->cpu->H | (1 << 1);
            cycles = 8;
            break;
        case SET_2_H:
            LOG_INFO("SET 2,H");
            gb->cpu->H = gb->cpu->H | (1 << 2);
            cycles = 8;
            break;
        case SET_3_H:
            LOG_INFO("SET 3,H");
            gb->cpu->H = gb->cpu->H | (1 << 3);
            cycles = 8;
            break;
        case SET_4_H:
            LOG_INFO("SET 4,H");
            gb->cpu->H = gb->cpu->H | (1 << 4);
            cycles = 8;
            break;
        case SET_5_H:
            LOG_INFO("SET 5,H");
            gb->cpu->H = gb->cpu->H | (1 << 5);
            cycles = 8;
            break;
        case SET_6_H:
            LOG_INFO("SET 6,H");
            gb->cpu->H = gb->cpu->H | (1 << 6);
            cycles = 8;
            break;
        case SET_7_H:
            LOG_INFO("SET 7,H");
            gb->cpu->H = gb->cpu->H | (1 << 7);
            cycles = 8;
            break;

        case SET_0_L:
            LOG_INFO("SET 0,L");
            gb->cpu->L = gb->cpu->L | (1 << 0);
            cycles = 8;
            break;
        case SET_1_L:
            LOG_INFO("SET 1,L");
            gb->cpu->L = gb->cpu->L | (1 << 1);
            cycles = 8;
            break;
        case SET_2_L:
            LOG_INFO("SET 2,L");
            gb->cpu->L = gb->cpu->L | (1 << 2);
            cycles = 8;
            break;
        case SET_3_L:
            LOG_INFO("SET 3,L");
            gb->cpu->L = gb->cpu->L | (1 << 3);
            cycles = 8;
            break;
        case SET_4_L:
            LOG_INFO("SET 4,L");
            gb->cpu->L = gb->cpu->L | (1 << 4);
            cycles = 8;
            break;
        case SET_5_L:
            LOG_INFO("SET 5,L");
            gb->cpu->L = gb->cpu->L | (1 << 5);
            cycles = 8;
            break;
        case SET_6_L:
            LOG_INFO("SET 6,L");
            gb->cpu->L = gb->cpu->L | (1 << 6);
            cycles = 8;
            break;
        case SET_7_L:
            LOG_INFO("SET 7,L");
            gb->cpu->L = gb->cpu->L | (1 << 7);
            cycles = 8;
            break;

        case SET_0_HL:
        {
            LOG_INFO("SET 0,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value | (1 << 0));
            cycles = 16;
            break;
        }
        case SET_1_HL:
        {
            LOG_INFO("SET 1,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value | (1 << 1));
            cycles = 16;
            break;
        }
        case SET_2_HL:
        {
            LOG_INFO("SET 2,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value | (1 << 2));
            cycles = 16;
            break;
        }
        case SET_3_HL:
        {
            LOG_INFO("SET 3,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value | (1 << 3));
            cycles = 16;
            break;
        }
        case SET_4_HL:
        {
            LOG_INFO("SET 4,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value | (1 << 4));
            cycles = 16;
            break;
        }
        case SET_5_HL:
        {
            LOG_INFO("SET 5,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value | (1 << 5));
            cycles = 16;
            break;
        }
        case SET_6_HL:
        {
            LOG_INFO("SET 6,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value | (1 << 6));
            cycles = 16;
            break;
        }
        case SET_7_HL:
        {
            LOG_INFO("SET 7,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value | (1 << 7));
            cycles = 16;
            break;
        }

        case SET_0_A:
            LOG_INFO("SET 0,A");
            gb->cpu->A = gb->cpu->A | (1 << 0);
            cycles = 8;
            break;
        case SET_1_A:
            LOG_INFO("SET 1,A");
            gb->cpu->A = gb->cpu->A | (1 << 1);
            cycles = 8;
            break;
        case SET_2_A:
            LOG_INFO("SET 2,A");
            gb->cpu->A = gb->cpu->A | (1 << 2);
            cycles = 8;
            break;
        case SET_3_A:
            LOG_INFO("SET 3,A");
            gb->cpu->A = gb->cpu->A | (1 << 3);
            cycles = 8;
            break;
        case SET_4_A:
            LOG_INFO("SET 4,A");
            gb->cpu->A = gb->cpu->A | (1 << 4);
            cycles = 8;
            break;
        case SET_5_A:
            LOG_INFO("SET 5,A");
            gb->cpu->A = gb->cpu->A | (1 << 5);
            cycles = 8;
            break;
        case SET_6_A:
            LOG_INFO("SET 6,A");
            gb->cpu->A = gb->cpu->A | (1 << 6);
            cycles = 8;
            break;
        case SET_7_A:
            LOG_INFO("SET 7,A");
            gb->cpu->A = gb->cpu->A | (1 << 7);
            cycles = 8;
            break;


       case RES_0_B:
            LOG_INFO("RES 0,B");
            gb->cpu->B = gb->cpu->B & ~(1 << 0);
            cycles = 8;
            break;
        case RES_1_B:
            LOG_INFO("RES 1,B");
            gb->cpu->B = gb->cpu->B & ~(1 << 1);
            cycles = 8;
            break;
        case RES_2_B:
            LOG_INFO("RES 2,B");
            gb->cpu->B = gb->cpu->B & ~(1 << 2);
            cycles = 8;
            break;
        case RES_3_B:
            LOG_INFO("RES 3,B");
            gb->cpu->B = gb->cpu->B & ~(1 << 3);
            cycles = 8;
            break;
        case RES_4_B:
            LOG_INFO("RES 4,B");
            gb->cpu->B = gb->cpu->B & ~(1 << 4);
            cycles = 8;
            break;
        case RES_5_B:
            LOG_INFO("RES 5,B");
            gb->cpu->B = gb->cpu->B & ~(1 << 5);
            cycles = 8;
            break;
        case RES_6_B:
            LOG_INFO("RES 6,B");
            gb->cpu->B = gb->cpu->B & ~(1 << 6);
            cycles = 8;
            break;
        case RES_7_B:
            LOG_INFO("RES 7,B");
            gb->cpu->B = gb->cpu->B & ~(1 << 7);
            cycles = 8;
            break;

        case RES_0_C:
            LOG_INFO("RES 0,C");
            gb->cpu->C = gb->cpu->C & ~(1 << 0);
            cycles = 8;
            break;
        case RES_1_C:
            LOG_INFO("RES 1,C");
            gb->cpu->C = gb->cpu->C & ~(1 << 1);
            cycles = 8;
            break;
        case RES_2_C:
            LOG_INFO("RES 2,C");
            gb->cpu->C = gb->cpu->C & ~(1 << 2);
            cycles = 8;
            break;
        case RES_3_C:
            LOG_INFO("RES 3,C");
            gb->cpu->C = gb->cpu->C & ~(1 << 3);
            cycles = 8;
            break;
        case RES_4_C:
            LOG_INFO("RES 4,C");
            gb->cpu->C = gb->cpu->C & ~(1 << 4);
            cycles = 8;
            break;
        case RES_5_C:
            LOG_INFO("RES 5,C");
            gb->cpu->C = gb->cpu->C & ~(1 << 5);
            cycles = 8;
            break;
        case RES_6_C:
            LOG_INFO("RES 6,C");
            gb->cpu->C = gb->cpu->C & ~(1 << 6);
            cycles = 8;
            break;
        case RES_7_C:
            LOG_INFO("RES 7,C");
            gb->cpu->C = gb->cpu->C & ~(1 << 7);
            cycles = 8;
            break;

        case RES_0_D:
            LOG_INFO("RES 0,D");
            gb->cpu->D = gb->cpu->D & ~(1 << 0);
            cycles = 8;
            break;
        case RES_1_D:
            LOG_INFO("RES 1,D");
            gb->cpu->D = gb->cpu->D & ~(1 << 1);
            cycles = 8;
            break;
        case RES_2_D:
            LOG_INFO("RES 2,D");
            gb->cpu->D = gb->cpu->D & ~(1 << 2);
            cycles = 8;
            break;
        case RES_3_D:
            LOG_INFO("RES 3,D");
            gb->cpu->D = gb->cpu->D & ~(1 << 3);
            cycles = 8;
            break;
        case RES_4_D:
            LOG_INFO("RES 4,D");
            gb->cpu->D = gb->cpu->D & ~(1 << 4);
            cycles = 8;
            break;
        case RES_5_D:
            LOG_INFO("RES 5,D");
            gb->cpu->D = gb->cpu->D & ~(1 << 5);
            cycles = 8;
            break;
        case RES_6_D:
            LOG_INFO("RES 6,D");
            gb->cpu->D = gb->cpu->D & ~(1 << 6);
            cycles = 8;
            break;
        case RES_7_D:
            LOG_INFO("RES 7,D");
            gb->cpu->D = gb->cpu->D & ~(1 << 7);
            cycles = 8;
            break;

        case RES_0_E:
            LOG_INFO("RES 0,E");
            gb->cpu->E = gb->cpu->E & ~(1 << 0);
            cycles = 8;
            break;
        case RES_1_E:
            LOG_INFO("RES 1,E");
            gb->cpu->E = gb->cpu->E & ~(1 << 1);
            cycles = 8;
            break;
        case RES_2_E:
            LOG_INFO("RES 2,E");
            gb->cpu->E = gb->cpu->E & ~(1 << 2);
            cycles = 8;
            break;
        case RES_3_E:
            LOG_INFO("RES 3,E");
            gb->cpu->E = gb->cpu->E & ~(1 << 3);
            cycles = 8;
            break;
        case RES_4_E:
            LOG_INFO("RES 4,E");
            gb->cpu->E = gb->cpu->E & ~(1 << 4);
            cycles = 8;
            break;
        case RES_5_E:
            LOG_INFO("RES 5,E");
            gb->cpu->E = gb->cpu->E & ~(1 << 5);
            cycles = 8;
            break;
        case RES_6_E:
            LOG_INFO("RES 6,E");
            gb->cpu->E = gb->cpu->E & ~(1 << 6);
            cycles = 8;
            break;
        case RES_7_E:
            LOG_INFO("RES 7,E");
            gb->cpu->E = gb->cpu->E & ~(1 << 7);
            cycles = 8;
            break;

        case RES_0_H:
            LOG_INFO("RES 0,H");
            gb->cpu->H = gb->cpu->H & ~(1 << 0);
            cycles = 8;
            break;
        case RES_1_H:
            LOG_INFO("RES 1,H");
            gb->cpu->H = gb->cpu->H & ~(1 << 1);
            cycles = 8;
            break;
        case RES_2_H:
            LOG_INFO("RES 2,H");
            gb->cpu->H = gb->cpu->H & ~(1 << 2);
            cycles = 8;
            break;
        case RES_3_H:
            LOG_INFO("RES 3,H");
            gb->cpu->H = gb->cpu->H & ~(1 << 3);
            cycles = 8;
            break;
        case RES_4_H:
            LOG_INFO("RES 4,H");
            gb->cpu->H = gb->cpu->H & ~(1 << 4);
            cycles = 8;
            break;
        case RES_5_H:
            LOG_INFO("RES 5,H");
            gb->cpu->H = gb->cpu->H & ~(1 << 5);
            cycles = 8;
            break;
        case RES_6_H:
            LOG_INFO("RES 6,H");
            gb->cpu->H = gb->cpu->H & ~(1 << 6);
            cycles = 8;
            break;
        case RES_7_H:
            LOG_INFO("RES 7,H");
            gb->cpu->H = gb->cpu->H & ~(1 << 7);
            cycles = 8;
            break;

        case RES_0_L:
            LOG_INFO("RES 0,L");
            gb->cpu->L = gb->cpu->L & ~(1 << 0);
            cycles = 8;
            break;
        case RES_1_L:
            LOG_INFO("RES 1,L");
            gb->cpu->L = gb->cpu->L & ~(1 << 1);
            cycles = 8;
            break;
        case RES_2_L:
            LOG_INFO("RES 2,L");
            gb->cpu->L = gb->cpu->L & ~(1 << 2);
            cycles = 8;
            break;
        case RES_3_L:
            LOG_INFO("RES 3,L");
            gb->cpu->L = gb->cpu->L & ~(1 << 3);
            cycles = 8;
            break;
        case RES_4_L:
            LOG_INFO("RES 4,L");
            gb->cpu->L = gb->cpu->L & ~(1 << 4);
            cycles = 8;
            break;
        case RES_5_L:
            LOG_INFO("RES 5,L");
            gb->cpu->L = gb->cpu->L & ~(1 << 5);
            cycles = 8;
            break;
        case RES_6_L:
            LOG_INFO("RES 6,L");
            gb->cpu->L = gb->cpu->L & ~(1 << 6);
            cycles = 8;
            break;
        case RES_7_L:
            LOG_INFO("RES 7,L");
            gb->cpu->L = gb->cpu->L & ~(1 << 7);
            cycles = 8;
            break;

        case RES_0_HL:
        {
            LOG_INFO("RES 0,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value & ~(1 << 0));
            cycles = 16;
            break;
        }
        case RES_1_HL:
        {
            LOG_INFO("RES 1,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value & ~(1 << 1));
            cycles = 16;
            break;
        }
        case RES_2_HL:
        {
            LOG_INFO("RES 2,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value & ~(1 << 2));
            cycles = 16;
            break;
        }
        case RES_3_HL:
        {
            LOG_INFO("RES 3,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value & ~(1 << 3));
            cycles = 16;
            break;
        }
        case RES_4_HL:
        {
            LOG_INFO("RES 4,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value & ~(1 << 4));
            cycles = 16;
            break;
        }
        case RES_5_HL:
        {
            LOG_INFO("RES 5,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value & ~(1 << 5));
            cycles = 16;
            break;
        }
        case RES_6_HL:
        {
            LOG_INFO("RES 6,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value & ~(1 << 6));
            cycles = 16;
            break;
        }
        case RES_7_HL:
        {
            LOG_INFO("RES 7,(HL)");
            uint8_t value = memory_get8(gb, cpu_get_value_HL(gb->cpu));
            memory_set8(gb, cpu_get_value_HL(gb->cpu), value & ~(1 << 7));
            cycles = 16;
            break;
        }

        case RES_0_A:
            LOG_INFO("RES 0,A");
            gb->cpu->A = gb->cpu->A & ~(1 << 0);
            cycles = 8;
            break;
        case RES_1_A:
            LOG_INFO("RES 1,A");
            gb->cpu->A = gb->cpu->A & ~(1 << 1);
            cycles = 8;
            break;
        case RES_2_A:
            LOG_INFO("RES 2,A");
            gb->cpu->A = gb->cpu->A & ~(1 << 2);
            cycles = 8;
            break;
        case RES_3_A:
            LOG_INFO("RES 3,A");
            gb->cpu->A = gb->cpu->A & ~(1 << 3);
            cycles = 8;
            break;
        case RES_4_A:
            LOG_INFO("RES 4,A");
            gb->cpu->A = gb->cpu->A & ~(1 << 4);
            cycles = 8;
            break;
        case RES_5_A:
            LOG_INFO("RES 5,A");
            gb->cpu->A = gb->cpu->A & ~(1 << 5);
            cycles = 8;
            break;
        case RES_6_A:
            LOG_INFO("RES 6,A");
            gb->cpu->A = gb->cpu->A & ~(1 << 6);
            cycles = 8;
            break;
        case RES_7_A:
            LOG_INFO("RES 7,A");
            gb->cpu->A = gb->cpu->A & ~(1 << 7);
            cycles = 8;
            break;


        default:
            LOG_ERROR("Base %x not found for prefix 0xCB.", base);
            exit(1);
    }
    return cycles;
}
