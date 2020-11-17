#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "screen.h"

static const pallet_bitmask_map[4] = {0b00000011, 0b00001100, 0b00110000, 0b11000000};

void screen_update_tiles(uint8_t* gb_memory, uint8_t* frame_buffer) {
    uint16_t tile_data = 0;
    uint16_t background_memory = 0;
    bool is_unsigned = true;
    bool is_using_window = false;
    // printf("yay!\n");
    uint8_t scroll_y = gb_memory[0xFF42];
    uint8_t scroll_x = gb_memory[0xFF43];
    uint8_t window_y = gb_memory[0xFF4A];
    uint8_t window_x = gb_memory[0xFF4B] - 7;

    uint8_t lcd_control = gb_memory[0xFF40];

    if (lcd_control & (1 << 5)) {
        if (window_y <= gb_memory[0xFF44]) is_using_window = true;
    }

    if (lcd_control & (1 << 4)) {
        tile_data = 0x8000;
    } else {
        tile_data = 0x8800;
        is_unsigned = false;
    }

    if (!is_using_window) {
        if (lcd_control & (1 << 3)) {
            background_memory = 0x9C00;
        } else {
            background_memory = 0x9800;
        }
    } else {
        if (lcd_control & (1 << 6)) {
            background_memory = 0x9C00;
        } else {
            background_memory = 0x9800;
        }
    }

    uint8_t y_pos = 0;

    if (!is_using_window) {
        y_pos = scroll_y + gb_memory[0xFF44];
    } else {
        y_pos = gb_memory[0xFF44] - window_y;
    }

    uint16_t tile_row = ((y_pos / 8)) * 32;  // Base row index of the tile the scan line is on.

    uint8_t x_pos;
    uint16_t tile_col;
    uint16_t tile_address;
    int16_t tile_num;
    uint16_t tile_location;
    uint8_t line;

    uint8_t data1;
    uint8_t data2;

    uint8_t color_offset;
    uint8_t color_num;
    uint8_t color;
    
    uint8_t y;
    for (uint8_t x = 0; x < 160; x++) {
        if (is_using_window && x >= window_x) {
            x_pos = x - window_x;
        } else {
            x_pos = x + scroll_x;
        }

        tile_col = x_pos / 8;

        tile_address = background_memory + tile_row + tile_col;
        if (is_unsigned) {
            tile_num = (uint8_t) gb_memory[tile_address];
            tile_location = tile_data + (tile_num*16);
        } else {
            tile_num = (int8_t) gb_memory[tile_address];
            tile_location = tile_data + ((tile_num+128)*16);
        }

        line = y_pos % 8;
        line = line * 2;

        data1 = gb_memory[tile_location+line];
        data2 = gb_memory[tile_location+line+1];

        color_offset = 7 - (x_pos % 8);
        color_num = (((data2 >> color_offset) & 0x01) << 1) | ((data1 >> color_offset) & 0x01);
        
        y = gb_memory[0xFF44];
        if (y > 143 || x > 159) {
            continue;
        }

        color = (pallet_bitmask_map[color_num] & gb_memory[0xFF47]) >> (color_num * 2);

        // printf("x = %d, y = %d\n", x, y);

        switch (color) {
            case 0:
                frame_buffer[3*(y*160+x)] = 255;
                frame_buffer[3*(y*160+x)+1] = 255;
                frame_buffer[3*(y*160+x)+2] = 255;
                break;
            case 1:
                frame_buffer[3*(y*160+x)] = 170;
                frame_buffer[3*(y*160+x)+1] = 170;
                frame_buffer[3*(y*160+x)+2] = 170;
                break;
            case 2:
                frame_buffer[3*(y*160+x)] = 85;
                frame_buffer[3*(y*160+x)+1] = 85;
                frame_buffer[3*(y*160+x)+2] = 85;
                break;
            case 3:
                frame_buffer[3*(y*160+x)] = 0;
                frame_buffer[3*(y*160+x)+1] = 0;
                frame_buffer[3*(y*160+x)+2] = 0;
                break;
        }
    }

    
}


void screen_update_sprites(uint8_t* gb_memory, uint8_t* frame_buffer) {
    
}


void screen_scanline_update(uint8_t* gb_memory, uint8_t* frame_buffer) {
    uint8_t lcd_control = gb_memory[0xFF40];

    if (lcd_control & 0x01) screen_update_tiles(gb_memory, frame_buffer);
    if ((lcd_control >> 1) & 0x01) screen_update_sprites(gb_memory, frame_buffer);

    gb_memory[0xFF44] = (gb_memory[0xFF44] + 1) % 154;
    // TODO(mct): V blank interupt.


}