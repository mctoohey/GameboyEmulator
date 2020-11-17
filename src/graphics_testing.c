#include <windows.h>
#include <stdint.h>
#include <stdio.h>

struct {
    uint16_t width, height;
    uint8_t *pixels;
    BITMAPINFO bitmap_info;
} typedef RenderBuffer;

RenderBuffer render_buffer;

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

            // Fiell bitmap_info
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

            // Testing
            uint32_t i = 0;
            for (uint32_t y = 0; y < render_buffer.height; y++) {
                for (uint32_t x = 0; x < render_buffer.width; x++) {
                    render_buffer.pixels[i] = 0xFF;
                    render_buffer.pixels[i+2] = 0xFF;
                    i += 3;
                }
            }

            break;
        }
        default:
            result = DefWindowProcA(window, message, w_param, l_param);
    }
    return result;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    WNDCLASSA window_class = {0};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpszClassName = "GBC_Window_Class";
    window_class.lpfnWndProc = window_callback;

    RegisterClassA(&window_class);

    HWND window =  CreateWindowExA(0, window_class.lpszClassName, "GBC",
                            WS_VISIBLE|WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                            1280, 720, 0, 0, 0, 0);

    HDC hdc = GetDC(window);
    while (running) {
        // Input
        MSG message;
        while (PeekMessageA(&message, window, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        // Simulation

        // Render
        StretchDIBits(hdc, 0, 0, render_buffer.width, render_buffer.height, 0, 0,
                    render_buffer.width, render_buffer.height, render_buffer.pixels,
                    &render_buffer.bitmap_info, DIB_RGB_COLORS, SRCCOPY);
    }

}
