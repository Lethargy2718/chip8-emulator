#include "chip8.hpp"

void chip8::start() {
    initialize_font();
}

void chip8::initialize_font() {
    for (uint16_t current = font_start; current < font_start + font_size; current++) {
        memory[current] = font[current - font_start];
    }
}

void chip8::tick_timers() {
    if (delay_timer > 0) delay_timer--;
    if (sound_timer > 0) sound_timer--;
}



