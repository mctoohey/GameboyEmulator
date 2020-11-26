#ifndef SRC_SCREEN_H_
#define SRC_SCREEN_H_

#include <stdint.h>

void screen_scanline_update(uint8_t* gb_memory, uint8_t* frame_buffer);

#endif  // SRC_SCREEN_H_