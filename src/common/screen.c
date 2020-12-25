#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "screen.h"

#define WHITE 0
#define LIGHT_GREY 1
#define DARK_GREY 2
#define BLACK 3

static const uint8_t pallet_bitmask_map[4] = {0b00000011, 0b00001100, 0b00110000, 0b11000000};

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
            case WHITE:
                frame_buffer[3*(y*160+x)] = 255;
                frame_buffer[3*(y*160+x)+1] = 255;
                frame_buffer[3*(y*160+x)+2] = 255;
                break;
            case LIGHT_GREY:
                frame_buffer[3*(y*160+x)] = 170;
                frame_buffer[3*(y*160+x)+1] = 170;
                frame_buffer[3*(y*160+x)+2] = 170;
                break;
            case DARK_GREY:
                frame_buffer[3*(y*160+x)] = 85;
                frame_buffer[3*(y*160+x)+1] = 85;
                frame_buffer[3*(y*160+x)+2] = 85;
                break;
            case BLACK:
                frame_buffer[3*(y*160+x)] = 0;
                frame_buffer[3*(y*160+x)+1] = 0;
                frame_buffer[3*(y*160+x)+2] = 0;
                break;
        }
    }
}


void screen_update_sprites(uint8_t* gb_memory, uint8_t* frame_buffer) {
    uint8_t lcd_control = gb_memory[0xFF40];
    uint8_t sprite_height = 8;

    if (lcd_control & (1 << 2)) sprite_height = 16;

    for (uint8_t sprite_num = 0; sprite_num < 40; sprite_num++) {
        uint8_t i = sprite_num*4;
        uint8_t y_pos = gb_memory[0xFE00+i] - 16;
        uint8_t x_pos = gb_memory[0xFE00+i+1] - 8;
        uint8_t tile_location = gb_memory[0xFE00+i+2];
        uint8_t attributes = gb_memory[0xFE00+i+3];

        uint8_t scanline_pos = gb_memory[0xFF44];
        // printf("scan, ypos: %d, %d\n", scanline_pos, y_pos);
        if (scanline_pos >= y_pos && scanline_pos < y_pos+sprite_height) {
            uint8_t sprite_line = scanline_pos - y_pos;

            if (attributes & (1 << 6)) {
                sprite_line = sprite_height - sprite_line;
            }

            uint16_t data_address = 0x8000 + (tile_location * 16) + sprite_line*2;
            uint8_t data1 = gb_memory[data_address];
            uint8_t data2 = gb_memory[data_address+1];

            for (int8_t sprite_x = 7; sprite_x >= 0; sprite_x--) {
                int8_t color_bit = sprite_x;
                if (attributes & (1 << 5)) {
                    color_bit = 7 - color_bit;
                }

                uint8_t color_num = (((data2 >> color_bit) & 0x01) << 1) | ((data1 >> color_bit) & 0x01);
                uint16_t pallet_address = attributes & (1 << 4) ? 0xFF49 : 0xFF48;
                uint8_t color = (pallet_bitmask_map[color_num] & gb_memory[pallet_address]) >> (color_num * 2);

                if (color_num == WHITE) {
                    continue;
                }

                uint8_t x = x_pos + (7-sprite_x);

                if ((scanline_pos > 143) || x > 159) {
                    continue;
                }

                switch (color) {
                    case WHITE:
                        frame_buffer[3*(scanline_pos*160+x)] = 255;
                        frame_buffer[3*(scanline_pos*160+x)+1] = 255;
                        frame_buffer[3*(scanline_pos*160+x)+2] = 255;
                        break;
                    case LIGHT_GREY:
                        frame_buffer[3*(scanline_pos*160+x)] = 170;
                        frame_buffer[3*(scanline_pos*160+x)+1] = 170;
                        frame_buffer[3*(scanline_pos*160+x)+2] = 170;
                        break;
                    case DARK_GREY:
                        frame_buffer[3*(scanline_pos*160+x)] = 85;
                        frame_buffer[3*(scanline_pos*160+x)+1] = 85;
                        frame_buffer[3*(scanline_pos*160+x)+2] = 85;
                        break;
                    case BLACK:
                        frame_buffer[3*(scanline_pos*160+x)] = 0;
                        frame_buffer[3*(scanline_pos*160+x)+1] = 0;
                        frame_buffer[3*(scanline_pos*160+x)+2] = 0;
                        break;
                }

            }


        }
    }
}


void screen_scanline_update(uint8_t* gb_memory, uint8_t* frame_buffer) {
    uint8_t lcd_control = gb_memory[0xFF40];

    if (lcd_control & 0x01) screen_update_tiles(gb_memory, frame_buffer);
    if ((lcd_control >> 1) & 0x01) screen_update_sprites(gb_memory, frame_buffer);

    gb_memory[0xFF44] = (gb_memory[0xFF44] + 1) % 154;

    // V-blank interupt.
    if (gb_memory[0xFF44] == 145) gb_memory[0xFF0f] |= 0x01;


}