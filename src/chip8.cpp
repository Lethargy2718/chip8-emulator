#include <cstring>
#include <fstream>
#include <iostream>

#include "chip8.hpp"


bool chip8::load_rom(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate); // Read in binary mode with cursor at end

    // Failed
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open ROM: " + filename + " doesn't exist or is empty.");
    }

    constexpr size_t max_rom_size = 4096 - 0x200; // Available space
    std::streamsize size = file.tellg(); // File end location (which is file size)

    if (size > max_rom_size) {
        throw std::runtime_error("Failed to load ROM: " + filename + " is too large.");
    }

    file.seekg(0, std::ios::beg); // Move pointer back to beginning

    file.read(reinterpret_cast<char*>(&memory[0x200]), size); // Load into memory

    return true;
}

void chip8::start(const std::string& filename) {
    last_frame = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    initialize_font();

    if (!load_rom(filename)) {
    }
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

void chip8::tick() {
    double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();

    double dt = now - last_frame;
    last_frame = now;

    cpu_accumulator += dt;
    timer_accumulator += dt;

    while (timer_accumulator >= timer_dt) {
        tick_timers();
        timer_accumulator -= timer_dt;
    }

    // Cap cycles to prevent spikes
    int cycles = 0;
    while (cpu_accumulator >= cpu_dt && cycles < 15) {
        decode_and_execute(fetch());
        cpu_accumulator -= cpu_dt;
        cycles++;
    }
}

uint16_t chip8::fetch() {
    uint16_t instruction = (memory[pc] << 8) | memory[pc + 1];
    pc += 2;
    return instruction;
}

void chip8::decode_and_execute(uint16_t instruction) {
    uint8_t first_nibble = (0xF000 & instruction) >> 12;
    uint8_t X = (0x0F00 & instruction) >> 8;
    uint8_t Y = (0x00F0 & instruction) >> 4;
    uint8_t N = (0x000F & instruction);
    uint8_t NN = (0x00FF & instruction);
    uint16_t NNN = (0x0FFF & instruction);

    switch (first_nibble) {
        case 0x0: {
            if (instruction == 0x00E0) {
                // Clear screen
                memset(display, 0, sizeof(display));
            }
            break;
        }
        case 0x1: {
            // Jump
            pc = NNN;
            break;
        }
        case 0x6: {
            // Set v[X]
            v[X] = NN;
            break;
        }
        case 0x7: {
            // Add to v[X]
            v[X] += NN;
            break;
        }
        case 0xA: {
            // Set I
            I = NNN;
            break;
        }
        case 0xD: {
          // Draw N lines. Start reading from memory[I], and start
          // writing from row Y, col X
            uint8_t startCol = v[X], startRow = v[Y], height = N;
            v[0xF] = 0;
            for (uint8_t i = 0; i < height; i++) {
                uint8_t row = (startRow + i) % display_height;
                uint8_t data = memory[I + i];
                for (int j = 0; j < 8; j++) {
                    uint8_t col = (startCol + j) % display_width;
                    uint8_t pixel = (data >> (7 - j)) & 1;
                    if (pixel && display[row * display_width + col]) v[0xF] = 1;
                    display[row * display_width + col] ^= pixel;
                }
            }

            display_changed = true;
            break;
        }
        default:
            break;
    }
}