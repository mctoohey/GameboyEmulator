#ifndef SRC_MEMORY_H_
#define SRC_MEMORY_H_

#include <stdint.h>
#include "gameboy.h"



uint8_t memory_get8(Gameboy* gb, uint16_t address);

void memory_set8(Gameboy* gb, uint16_t address, uint8_t value);

uint16_t memory_get16(Gameboy* gb, uint16_t address);

void memory_set16(Gameboy* gb, uint16_t address, uint16_t value);

#endif  // SRC_MEMORY_H_


