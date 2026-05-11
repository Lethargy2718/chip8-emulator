#include <SDL2/SDL.h>
#include <cstdint>
#include <chrono>

#include "chip8.hpp"

void audio_callback(void* userdata, uint8_t* stream, int len) {
    static int sample = 0;
    for (int i = 0; i < len; i++) {
        stream[i] = (sample++ / 20 % 2) ? 128 + 64 : 128 - 64;
    }
}

int main() {
    // Init SDL
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    SDL_Window* window = SDL_CreateWindow(
        "CHIP-8",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 320,  // 64*10 and 32*10, scaled up
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

    // Timer
    constexpr std::chrono::duration<float> timer_interval{chip8::clock_interval};
    auto last_time = std::chrono::steady_clock::now();

    // Chip-8
    chip8 chip8;
    chip8.start();

    // Loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - last_time;

        if (elapsed >= timer_interval) {
            last_time = now;
            chip8.tick_timers();
        }

        SDL_PauseAudioDevice(audio_device, chip8.is_playing_sound() ? 0 : 1);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

