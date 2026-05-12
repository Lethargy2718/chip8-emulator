#include <SDL2/SDL.h>
#include <cstdint>
#include <chrono>

#include "chip8.hpp"

constexpr std::string rom_filename = "../ROMS/ibm.ch8";
constexpr int render_scale = 10;

void audio_callback(void* _, uint8_t* stream, const int len) {
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
    for (int y = 0; y < chip8::display_height; y++) {
        for (int x = 0; x < chip8::display_width; x++) {
            if (display[y * chip8::display_width + x]) {
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
        chip8::display_width * render_scale, chip8::display_height * render_scale,
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
    chip8 chip8;
    chip8.start(rom_filename);

    // Loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                bool pressed = event.type == SDL_KEYDOWN;
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_1: chip8.keys[0x1] = pressed; break;
                    case SDL_SCANCODE_2: chip8.keys[0x2] = pressed; break;
                    case SDL_SCANCODE_3: chip8.keys[0x3] = pressed; break;
                    case SDL_SCANCODE_4: chip8.keys[0xC] = pressed; break;
                    case SDL_SCANCODE_Q: chip8.keys[0x4] = pressed; break;
                    case SDL_SCANCODE_W: chip8.keys[0x5] = pressed; break;
                    case SDL_SCANCODE_E: chip8.keys[0x6] = pressed; break;
                    case SDL_SCANCODE_R: chip8.keys[0xD] = pressed; break;
                    case SDL_SCANCODE_A: chip8.keys[0x7] = pressed; break;
                    case SDL_SCANCODE_S: chip8.keys[0x8] = pressed; break;
                    case SDL_SCANCODE_D: chip8.keys[0x9] = pressed; break;
                    case SDL_SCANCODE_F: chip8.keys[0xE] = pressed; break;
                    case SDL_SCANCODE_Z: chip8.keys[0xA] = pressed; break;
                    case SDL_SCANCODE_X: chip8.keys[0x0] = pressed; break;
                    case SDL_SCANCODE_C: chip8.keys[0xB] = pressed; break;
                    case SDL_SCANCODE_V: chip8.keys[0xF] = pressed; break;
                    default: break;
                }
            }
        }
        chip8.tick();
        SDL_PauseAudioDevice(audio_device, chip8.is_playing_sound() ? 0 : 1);
        if (chip8.should_display()) {
            render_display(renderer, chip8);
            chip8.reset_should_display();
        }
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

