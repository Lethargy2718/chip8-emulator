#include <SDL2/SDL.h>
#include <cstdint>
#include <chrono>

#include "chip8.hpp"

const std::string rom_filename = "../roms/Pong.ch8";
constexpr int render_scale = 35;

void audio_callback(void*, uint8_t* stream, const int len) {
    static int sample = 0;
    for (int i = 0; i < len; i++) {
        stream[i] = (sample++ / 20 % 2) ? 128 + 64 : 128 - 64;
    }
}

void render_display(SDL_Renderer* renderer, const chip8& chip) {
    const auto& display = chip.get_display();

    // Clear renderer
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Set pixel draw color
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Draw each pixel
    for (int y = 0; y < chip8::rows; y++) {
        for (int x = 0; x < chip8::cols; x++) {
            if (display[y * chip8::cols + x]) {
                SDL_Rect rect = { x * render_scale, y * render_scale, render_scale, render_scale };
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    // Render
    SDL_RenderPresent(renderer);
}

int main() {
    // Init SDL
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    SDL_Window* window = SDL_CreateWindow(
        "CHIP-8",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        chip8::cols * render_scale, chip8::rows * render_scale,
        0
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_AudioSpec spec;
    SDL_memset(&spec, 0, sizeof(spec));
    spec.freq = 44100;
    spec.format = AUDIO_U8;
    spec.channels = 1;
    spec.samples = 512;
    spec.callback = audio_callback;
    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);

    // Chip-8
    chip8 emu;
    emu.start(rom_filename);

    // Loop
    bool running = true;
    while (running) {
        emu.reset_keys_released();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                bool pressed = event.type == SDL_KEYDOWN;
                int key = -1;
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_1: key = 0x1; break;
                    case SDL_SCANCODE_2: key = 0x2; break;
                    case SDL_SCANCODE_3: key = 0x3; break;
                    case SDL_SCANCODE_4: key = 0xC; break;
                    case SDL_SCANCODE_Q: key = 0x4; break;
                    case SDL_SCANCODE_W: key = 0x5; break;
                    case SDL_SCANCODE_E: key = 0x6; break;
                    case SDL_SCANCODE_R: key = 0xD; break;
                    case SDL_SCANCODE_A: key = 0x7; break;
                    case SDL_SCANCODE_S: key = 0x8; break;
                    case SDL_SCANCODE_D: key = 0x9; break;
                    case SDL_SCANCODE_F: key = 0xE; break;
                    case SDL_SCANCODE_Z: key = 0xA; break;
                    case SDL_SCANCODE_X: key = 0x0; break;
                    case SDL_SCANCODE_C: key = 0xB; break;
                    case SDL_SCANCODE_V: key = 0xF; break;
                    default: break;
                }
                if (key != -1) {
                    if (pressed) {
                        emu.keys_held[key] = true;
                    } else {
                        emu.keys_held[key] = false;
                        emu.keys_released[key] = true;
                    }
                }
            }
        }
        emu.tick();
        SDL_PauseAudioDevice(audio_device, emu.is_playing_sound() ? 0 : 1);
        if (emu.should_display()) {
            render_display(renderer, emu);
            emu.reset_should_display();
        }
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_CloseAudioDevice(audio_device);
    SDL_Quit();
}

