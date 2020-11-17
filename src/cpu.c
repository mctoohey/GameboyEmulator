#include <stdint.h>
#include "cpu.h"

uint16_t cpu_get_value_AF(CPU* cpu) {
    return (cpu->A << 8) | cpu->F;
}

uint16_t cpu_get_value_BC(CPU* cpu) {
    return (cpu->B << 8) | cpu->C;
}

uint16_t cpu_get_value_DE(CPU* cpu) {
    return (cpu->D << 8) | cpu->E;
}

uint16_t cpu_get_value_HL(CPU* cpu) {
    return (cpu->H << 8) | cpu->L;
}

void cpu_set_value_AF(CPU* cpu, uint16_t value) {
    cpu->A = value >> 8;
    cpu->F = value & 0xFF;
}

void cpu_set_value_BC(CPU* cpu, uint16_t value) {
    cpu->B = value >> 8;
    cpu->C = value & 0xFF;
}

void cpu_set_value_DE(CPU* cpu, uint16_t value) {
    cpu->D = value >> 8;
    cpu->E = value & 0xFF;
}

void cpu_set_value_HL(CPU* cpu, uint16_t value) {
    cpu->H = value >> 8;
    cpu->L = value & 0xFF;
}

void cpu_increment_BC(CPU* cpu) {
    cpu->C++;
    if (!cpu->C) cpu->B++;
}

void cpu_increment_DE(CPU* cpu) {
    cpu->E++;
    if (!cpu->E) cpu->D++;
}

void cpu_increment_HL(CPU* cpu) {
    cpu->L++;
    if (!cpu->L) cpu->H++;
}

void cpu_decrement_BC(CPU* cpu) {
    cpu->C--;
    if (cpu->C == 0xFF) cpu->B--;
}

void cpu_decrement_DE(CPU* cpu) {
    cpu->E--;
    if (cpu->E == 0xFF) cpu->D--;
}

void cpu_decrement_HL(CPU* cpu) {
    cpu->L--;
    if (cpu->L == 0xFF) cpu->H--;
}

void cpu_flag_resetZ(CPU* cpu) {
    cpu->F &= ~(1 << 7);
}

void cpu_flag_resetN(CPU* cpu) {
    cpu->F &= ~(1 << 6);
}

void cpu_flag_resetH(CPU* cpu) {
    cpu->F &= ~(1 << 5);
}

void cpu_flag_resetC(CPU* cpu) {
    cpu->F &= ~(1 << 4);
}

void cpu_flag_setZ(CPU* cpu) {
    cpu->F |= (1 << 7);
}

void cpu_flag_setN(CPU* cpu) {
    cpu->F |= (1 << 6);
}

void cpu_flag_setH(CPU* cpu) {
    cpu->F |= (1 << 5);
}

void cpu_flag_setC(CPU* cpu) {
    cpu->F |= (1 << 4);
}

uint8_t cpu_flag_getZ(CPU* cpu) {
    return (cpu->F >> 7) & 0x01;
}

uint8_t cpu_flag_getC(CPU* cpu) {
    return (cpu->F >> 4) & 0x01;
}


void cpu_add_to_A(CPU* cpu, uint8_t value) {
    uint16_t result = cpu->A + value;

    cpu_flag_resetN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->A & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);
    result >> 8 ? cpu_flag_setC(cpu) : cpu_flag_resetC(cpu);

    cpu->A = (uint8_t) result;
    cpu->A ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
}

void cpu_addcarry_to_A(CPU* cpu, uint8_t value) {
    uint16_t result = cpu->A + value + cpu_flag_getC(cpu);

    cpu_flag_resetN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->A & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);
    result >> 8 ? cpu_flag_setC(cpu) : cpu_flag_resetC(cpu);

    cpu->A = (uint8_t) result;
    cpu->A ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
}

void cpu_subtract_from_A(CPU* cpu, uint8_t value) {
    uint16_t result = (1 << 8) + cpu->A - value;

    cpu_flag_setN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->A & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);
    result >> 8 ? cpu_flag_setC(cpu) : cpu_flag_resetC(cpu);

    cpu->A = (uint8_t) result;
    cpu->A ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
}

void cpu_subtractcarry_from_A(CPU* cpu, uint8_t value) {
    uint16_t result = (1 << 8) + cpu->A - value + cpu_flag_getC(cpu);

    cpu_flag_setN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->A & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);
    result >> 8 ? cpu_flag_setC(cpu) : cpu_flag_resetC(cpu);

    cpu->A = (uint8_t) result;
    cpu->A ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
}

void cpu_and_A(CPU* cpu, uint8_t value) {
    cpu->A = cpu->A & value;

    cpu->A ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    cpu_flag_setH(cpu);
    cpu_flag_resetC(cpu);
}

void cpu_or_A(CPU* cpu, uint8_t value) {
    cpu->A = cpu->A | value;

    cpu->A ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    cpu_flag_resetH(cpu);
    cpu_flag_resetC(cpu);
}

void cpu_xor_A(CPU* cpu, uint8_t value) {
    cpu->A = cpu->A ^ value;

    cpu->A ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    cpu_flag_resetH(cpu);
    cpu_flag_resetC(cpu);
}

void cpu_compare_A(CPU* cpu, uint8_t value) {
    uint16_t result = (1 << 8) + cpu->A - value;

    cpu_flag_setN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->A & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);
    result >> 8 ? cpu_flag_setC(cpu) : cpu_flag_resetC(cpu);

    ((uint8_t) result) ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
}



void cpu_increment_A(CPU* cpu) {
    uint8_t result = cpu->A + 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->A & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->A = result;
}

void cpu_increment_B(CPU* cpu) {
    uint8_t result = cpu->B + 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->B & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->B = result;
}

void cpu_increment_C(CPU* cpu) {
    uint8_t result = cpu->C + 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->C & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->C = result;
}

void cpu_increment_D(CPU* cpu) {
    uint8_t result = cpu->D + 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->D & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->D = result;
}

void cpu_increment_E(CPU* cpu) {
    uint8_t result = cpu->E + 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->E & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->E = result;
}

void cpu_increment_H(CPU* cpu) {
    uint8_t result = cpu->H + 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->H & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->H = result;
}

void cpu_increment_L(CPU* cpu) {
    uint8_t result = cpu->L + 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->L & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->L = result;
}

uint8_t cpu_increment8_value(CPU* cpu, uint8_t value) {
    uint8_t result = value + 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(value & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    return result;
}

void cpu_decrement_A(CPU* cpu) {
    uint8_t result = cpu->A - 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_setN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->A & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->A =  result;
}

void cpu_decrement_B(CPU* cpu) {
    uint8_t result = cpu->B - 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_setN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->B & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->B =  result;
}

void cpu_decrement_C(CPU* cpu) {
    uint8_t result = cpu->C - 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_setN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->C & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->C =  result;
}

void cpu_decrement_D(CPU* cpu) {
    uint8_t result = cpu->D - 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_setN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->D & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->D =  result;
}

void cpu_decrement_E(CPU* cpu) {
    uint8_t result = cpu->E - 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_setN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->E & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->E =  result;
}

void cpu_decrement_H(CPU* cpu) {
    uint8_t result = cpu->H - 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_setN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->H & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->H =  result;
}

void cpu_decrement_L(CPU* cpu) {
    uint8_t result = cpu->L - 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_setN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(cpu->L & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    cpu->L =  result;
}

uint8_t cpu_decrement8_value(CPU* cpu, uint8_t value) {
    uint8_t result = value - 1;

    result ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_setN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 4) && !(value & (1 << 4)) ? cpu_flag_setH(cpu) : cpu_flag_resetH(cpu);

    return result;
}



void cpu_add16_to_HL(CPU* cpu, uint16_t value) {
    uint32_t result = cpu_get_value_HL(cpu) + value;

    cpu_flag_resetN(cpu);
    // WARNING: May be incorrect.
    result & (1 << 12) && !(cpu_get_value_HL(cpu) & (1 << 12)) ? cpu_flag_setH(cpu) :
                                                                 cpu_flag_resetH(cpu);
    result >> 16 ? cpu_flag_setC(cpu) : cpu_flag_resetC(cpu);

    cpu_set_value_HL(cpu, (uint16_t) result);
}



uint8_t cpu_rlc_value(CPU* cpu, uint8_t value) {
    uint8_t end = value >> 7;
    value = (value << 1) | end;
    value ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    cpu_flag_resetH(cpu);
    end ? cpu_flag_setC(cpu) : cpu_flag_resetC(cpu);

    return value;
}

uint8_t cpu_rl_value(CPU* cpu, uint8_t value) {
    uint8_t end = value >> 7;
    value = (value << 1) | cpu_flag_getC(cpu);
    value ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    cpu_flag_resetH(cpu);
    end ? cpu_flag_setC(cpu) : cpu_flag_resetC(cpu);

    return value;
}

uint8_t cpu_rrc_value(CPU* cpu, uint8_t value) {
    uint8_t start = value & 0x01;
    value = (value >> 1) | (start << 7);
    value ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    cpu_flag_resetH(cpu);
    start ? cpu_flag_setC(cpu) : cpu_flag_resetC(cpu);

    return value;
}

uint8_t cpu_rr_value(CPU* cpu, uint8_t value) {
    uint8_t start = value & 0x01;
    value = (value >> 1) | (cpu_flag_getC(cpu) << 7);
    value ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    cpu_flag_resetH(cpu);
    start ? cpu_flag_setC(cpu) : cpu_flag_resetC(cpu);

    return value;
}

uint8_t cpu_sla_value(CPU* cpu, uint8_t value) {
    (value >> 7) ? cpu_flag_setC(cpu) : cpu_flag_resetC(cpu);
    value = value << 1;
    value ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    cpu_flag_resetH(cpu);

    return value;
}

uint8_t cpu_sra_value(CPU* cpu, uint8_t value) {
    (value & 0x01) ? cpu_flag_setC(cpu) : cpu_flag_resetC(cpu);
    value = (value >> 1) | (value & (1 << 7));
    value ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    cpu_flag_resetH(cpu);

    return value;
}

uint8_t cpu_srl_value(CPU* cpu, uint8_t value) {
    (value & 0x01) ? cpu_flag_setC(cpu) : cpu_flag_resetC(cpu);
    value = value >> 1;
    value ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    cpu_flag_resetH(cpu);

    return value;
}



void cpu_test_bit_value(CPU* cpu, uint8_t value, uint8_t n) {
    (value & (1 << n)) ? cpu_flag_resetZ(cpu) : cpu_flag_setZ(cpu);
    cpu_flag_resetN(cpu);
    cpu_flag_setH(cpu);
}
