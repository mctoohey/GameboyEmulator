#include <windows.h>
#include <stdint.h>
#include <stdio.h>

#include "gameboy.h"
#include "screen.h"
#include "cpu.h"

#define WIDTH 160
#define HEIGHT 144

typedef struct {
    uint16_t width, height;
    uint8_t *pixels;
    BITMAPINFO bitmap_info;
} RenderBuffer;

static RenderBuffer render_buffer;

static uint8_t running = 1;

LRESULT CALLBACK window_callback(
  HWND   window,
  UINT   message,
  WPARAM w_param,
  LPARAM l_param
) {
    LRESULT result = 0;
    switch (message) {
        case WM_CLOSE:
        case WM_DESTROY:
            running = 0;
            break;
        case WM_SIZE:
        {
            // Get width and height.
            RECT rect;
            GetWindowRect(window, &rect);
            render_buffer.width = rect.right - rect.left;
            render_buffer.height = rect.bottom - rect.top;

            if (render_buffer.pixels) {
                VirtualFree(render_buffer.pixels, 0, MEM_RELEASE);
            }

            render_buffer.pixels = VirtualAlloc(0,
                                    sizeof(uint8_t)*render_buffer.width*render_buffer.height*3,
                                    MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

            // Testing
            // uint32_t i = 0;
            // for (uint32_t y = 0; y < render_buffer.height; y++) {
            //     for (uint32_t x = 0; x < render_buffer.width; x++) {
            //         render_buffer.pixels[i] = 0xFF;
            //         render_buffer.pixels[i+2] = 0xFF;
            //         i += 3;
            //     }
            // }

            break;
        }
        default:
            result = DefWindowProcA(window, message, w_param, l_param);
    }
    return result;
}

void window_render_buffer_init(uint32_t width, uint32_t height) {
    render_buffer.width = width;
    render_buffer.height = height+1;
    render_buffer.pixels = VirtualAlloc(0,
                                    sizeof(uint8_t)*render_buffer.width*render_buffer.height*3,
                                    MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

    render_buffer.bitmap_info.bmiHeader.biSize = sizeof(render_buffer.bitmap_info.bmiHeader);
    render_buffer.bitmap_info.bmiHeader.biWidth = render_buffer.width;
    render_buffer.bitmap_info.bmiHeader.biHeight = render_buffer.height;
    render_buffer.bitmap_info.bmiHeader.biPlanes = 1;
    render_buffer.bitmap_info.bmiHeader.biBitCount = 24;
    render_buffer.bitmap_info.bmiHeader.biCompression = BI_RGB;
    render_buffer.bitmap_info.bmiHeader.biSizeImage = 0;
    render_buffer.bitmap_info.bmiHeader.biXPelsPerMeter = 0;
    render_buffer.bitmap_info.bmiHeader.biYPelsPerMeter = 0;
    render_buffer.bitmap_info.bmiHeader.biClrUsed = 0;
    render_buffer.bitmap_info.bmiHeader.biClrImportant = 0;
}

HWND window_init(uint32_t width, uint32_t height) {
    WNDCLASSA window_class = {0};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpszClassName = "GBC_Window_Class";
    window_class.lpfnWndProc = window_callback;

    RegisterClassA(&window_class);

    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = width;
    rect.bottom = height;
    AdjustWindowRectEx(&rect, WS_VISIBLE|WS_CAPTION|WS_MINIMIZEBOX|WS_SYSMENU, FALSE, 0);
    // printf("%d %d\n", rect.right - rect.left, rect.bottom - rect.top);
    HWND window =  CreateWindowExA(0, window_class.lpszClassName, "GBC",
                            WS_VISIBLE|WS_CAPTION|WS_MINIMIZEBOX|WS_SYSMENU,
                            CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left,
                            rect.bottom - rect.top, 0, 0, 0, 0);

    return window;
}

void window_render(HDC hdc) {
    StretchDIBits(hdc, 0, 0, render_buffer.width, render_buffer.height, 0, render_buffer.height,
                    render_buffer.width, -render_buffer.height, render_buffer.pixels,
                    &render_buffer.bitmap_info, DIB_RGB_COLORS, SRCCOPY);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    HWND window = window_init(WIDTH, HEIGHT);
    window_render_buffer_init(WIDTH, HEIGHT);
    printf("hi\n");

    CPU cpu;
    cpu.PC = 0;
    uint8_t memory[0x10000] = {0};
    uint8_t bootstrap_rom[0x100] = {0};
    Gameboy gb = {&cpu, memory, bootstrap_rom, 0};


    FILE* rom_fp = fopen("../../ROMS/drmario.gb", "rb");
    FILE* boostrap_fp = fopen("DMG_ROM.bin", "rb");

    gameboy_load_rom(&gb, rom_fp);
    gameboy_load_bootstrap(&gb, boostrap_fp);

    fclose(rom_fp);
    fclose(boostrap_fp);
    // memory[0xFF44] = 0x90;

    // for (int i = 0; i < 1000000; i++) gameboy_update(&gb, render_buffer.pixels);

    // for (int i = 0; i < 30; i++)
    //     printf("%x, %x\n", bootstrap_rom[0xa8+i], memory[0x104+i]);

    HDC hdc = GetDC(window);
    uint32_t i = 0;
    uint8_t buttons = 0xFF;
    while (running) {
        // Input
        MSG message;
        while (PeekMessageA(&message, window, 0, 0, PM_REMOVE)) {

            switch (message.message) {
                case WM_KEYDOWN:
                {
                    uint32_t vk_code = message.wParam;
                    // printf("down %d\n", vk_code);
                    if (vk_code == 'S') {
                        buttons &= ~(1 << 7);
                    } else if (vk_code == 'W') {
                        buttons &= ~(1 << 6);
                    } else if (vk_code == 'A') {
                        buttons &= ~(1 << 5);
                    } else if (vk_code == 'D') {
                        buttons &= ~(1 << 4);
                    } else if (vk_code == VK_RETURN) {
                        buttons &= ~(1 << 3);
                        // printf("down enter %x\n", buttons);
                    } else if (vk_code == VK_RSHIFT) {
                        buttons &= ~(1 << 2);
                    } else if (vk_code == 'E') {
                        buttons &= ~(1 << 1);
                    } else if (vk_code == VK_SPACE) {
                        buttons &= ~(0x01);
                    }
                    break;
                }
                case WM_KEYUP:
                {
                    uint32_t vk_code = message.wParam;
                    // printf("up\n");
                    if (vk_code == 'S') {
                        buttons |= (1 << 7);
                    } else if (vk_code == 'W') {
                        buttons |= (1 << 6);
                    } else if (vk_code == 'A') {
                        buttons |= (1 << 5);
                    } else if (vk_code == 'D') {
                        buttons |= (1 << 4);
                    } else if (vk_code == VK_RETURN) {
                        buttons |= (1 << 3);
                    } else if (vk_code == VK_RSHIFT) {
                        buttons |= (1 << 2);
                    } else if (vk_code == 'E') {
                        buttons |= (1 << 1);
                    } else if (vk_code == VK_SPACE) {
                        buttons |= (0x01);
                    }
                    break;
                }
                default:
                    TranslateMessage(&message);
                    DispatchMessage(&message);
            }
        }
        // printf("buttons %x\n", buttons);
        // Simulation
        gameboy_update(&gb, buttons);

        if (i % 5 == 0) {
            screen_scanline_update(gb.memory, render_buffer.pixels);
        }

        if (i % 50 == 0) {
            gb.memory[0xFF05]++;
            if (gb.memory[0xFF05] == 0) {
                gb.memory[0xFF0F] |= (1 << 2);
            }
        }
 
        // Render
        if (i % 2000 == 0) {
            window_render(hdc);
        }
        // Sleep(100);
        i++;
    }
    for (uint16_t i = 0xFE00; i <= 0xFE9F; i++) {
        printf("$%.2x\n", memory[i]);
    }
    return 0;
}
