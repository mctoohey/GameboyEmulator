#ifndef SRC_CPU_H_
#define SRC_CPU_H_

#include <stdint.h>

typedef struct cpu_t {
    uint8_t A;
    uint8_t F;
    uint8_t B;
    uint8_t C;
    uint8_t D;
    uint8_t E;
    uint8_t H;
    uint8_t L;
    uint16_t SP;
    uint16_t PC;
} CPU;

uint16_t cpu_get_value_AF(CPU* cpu);

uint16_t cpu_get_value_BC(CPU* cpu);

uint16_t cpu_get_value_DE(CPU* cpu);

uint16_t cpu_get_value_HL(CPU* cpu);

void cpu_set_value_AF(CPU* cpu, uint16_t value);

void cpu_set_value_BC(CPU* cpu, uint16_t value);

void cpu_set_value_DE(CPU* cpu, uint16_t value);

void cpu_set_value_HL(CPU* cpu, uint16_t value);



void cpu_increment_BC(CPU* cpu);
void cpu_increment_DE(CPU* cpu);
void cpu_increment_HL(CPU* cpu);

void cpu_decrement_BC(CPU* cpu);
void cpu_decrement_DE(CPU* cpu);
void cpu_decrement_HL(CPU* cpu);



void cpu_flag_resetZ(CPU* cpu);
void cpu_flag_resetN(CPU* cpu);
void cpu_flag_resetH(CPU* cpu);
void cpu_flag_resetC(CPU* cpu);

void cpu_flag_setZ(CPU* cpu);
void cpu_flag_setN(CPU* cpu);
void cpu_flag_setH(CPU* cpu);
void cpu_flag_setC(CPU* cpu);

uint8_t cpu_flag_getZ(CPU* cpu);
uint8_t cpu_flag_getN(CPU* cpu);
uint8_t cpu_flag_getH(CPU* cpu);
uint8_t cpu_flag_getC(CPU* cpu);



void cpu_add_to_A(CPU* cpu, uint8_t value);
void cpu_addcarry_to_A(CPU* cpu, uint8_t value);
void cpu_subtract_from_A(CPU* cpu, uint8_t value);
void cpu_subtractcarry_from_A(CPU* cpu, uint8_t value);

void cpu_and_A(CPU* cpu, uint8_t value);
void cpu_or_A(CPU* cpu, uint8_t value);
void cpu_xor_A(CPU* cpu, uint8_t value);

void cpu_compare_A(CPU* cpu, uint8_t value);



void cpu_increment_A(CPU* cpu);
void cpu_increment_B(CPU* cpu);
void cpu_increment_C(CPU* cpu);
void cpu_increment_D(CPU* cpu);
void cpu_increment_E(CPU* cpu);
void cpu_increment_H(CPU* cpu);
void cpu_increment_L(CPU* cpu);

uint8_t cpu_increment8_value(CPU* cpu, uint8_t value);

void cpu_decrement_A(CPU* cpu);
void cpu_decrement_B(CPU* cpu);
void cpu_decrement_C(CPU* cpu);
void cpu_decrement_D(CPU* cpu);
void cpu_decrement_E(CPU* cpu);
void cpu_decrement_H(CPU* cpu);
void cpu_decrement_L(CPU* cpu);

uint8_t cpu_decrement8_value(CPU* cpu, uint8_t value);



void cpu_add16_to_HL(CPU* cpu, uint16_t value);



uint8_t cpu_rlc_value(CPU* cpu, uint8_t value);
uint8_t cpu_rl_value(CPU* cpu, uint8_t value);
uint8_t cpu_rrc_value(CPU* cpu, uint8_t value);
uint8_t cpu_rr_value(CPU* cpu, uint8_t value);

uint8_t cpu_sla_value(CPU* cpu, uint8_t value);
uint8_t cpu_sra_value(CPU* cpu, uint8_t value);
uint8_t cpu_srl_value(CPU* cpu, uint8_t value);



void cpu_test_bit_value(CPU* cpu, uint8_t value, uint8_t n);



uint8_t cpu_swap_value(CPU* cpu, uint8_t value);



void cpu_daa(CPU* cpu);

#endif  // SRC_CPU_H_
