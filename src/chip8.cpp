#include <cstring>
#include <fstream>
#include <iostream>

#include "chip8.hpp"
#include "random.hpp"

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
    load_rom(filename);
}

void chip8::initialize_font() {
    for (uint16_t current = font_start; current < font_start + font_size; current++) {
        memory[current] = font[current - font_start];
    }
}

void chip8::tick_timers() {
    if (delay_timer > 0) delay_timer--;
    if (sound_timer > 0) sound_timer--;
	rendered = false;
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
    while (cpu_accumulator >= cpu_dt && cycles < 40) {
        decode_and_execute(fetch());
        cpu_accumulator -= cpu_dt;
        cycles++;
    }
}

void chip8::push(uint16_t i) {
    stack[sp++] = i;
}

uint16_t chip8::pop() {
    return stack[--sp];
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
                std::memset(display, 0, sizeof(display));
            }
            else if (instruction == 0x00EE) {
                // Return from subroutine
                pc = pop();
            }
            break;
        }
        case 0x1: {
            // Jump
            pc = NNN;
            break;
        }
        case 0x2: {
            // Call subroutine
            push(pc);
            pc = NNN;
            break;
        }
        case 0x3: {
            // Conditional skip
            if (v[X] == NN) {
                pc += 2;
            }
            break;
        }
        case 0x4: {
            // Conditional skip
            if (v[X] != NN) {
                pc += 2;
            }
            break;
        }
        case 0x5: {
            // Conditional skip
            if (v[X] == v[Y]) {
                pc += 2;
            }
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
        case 0x8: {
            switch (N) {
                case 0x0: {
                    // Set
                    v[X] = v[Y];
                    break;
                }
                case 0x1: {
                    // Binary OR
                    v[X] |= v[Y];
                    if (config.vf_reset) v[0xF] = 0;
                    break;
                }
                case 0x2: {
                    // Binary AND
                    v[X] &= v[Y];
                    if (config.vf_reset) v[0xF] = 0;
                    break;
                }
                case 0x3: {
                    // Binary XOR
                    v[X] ^= v[Y];
                    if (config.vf_reset) v[0xF] = 0;
                    break;
                }
                case 0x4: {
                    // Add
                    uint16_t sum = v[X] + v[Y];
                    v[X] = sum & 0x00FF;
                    v[0xF] = sum > 0x00FF; // Overflow
                    break;
                }
                case 0x5: {
                    // Subtract
                    uint8_t borrow = v[X] >= v[Y];
                    v[X] -= v[Y];
                    v[0xF] = borrow; // Borrow bit
                    break;
                }
                case 0x6: {
                    // Shift right
                    if (config.set_vx) v[X] = v[Y];
                    uint8_t bit = v[X] & 0x0001; // Shifted bit (LSB)
                    v[X] >>= 1;
                    v[0xF] = bit;
                    break;
                }
                case 0x7: {
                    // Subtract
                    uint8_t borrow = v[Y] >= v[X]; // Borrow bit
                    v[X] = v[Y] - v[X];
                    v[0xF] = borrow;
                    break;
                }
                case 0xE: {
                    // Shift left
                    if (config.set_vx) v[X] = v[Y];
                    uint8_t bit = (v[X] & 0x80) >> 7; // Shifted bit (MSB)
                    v[X] <<= 1;
                    v[0xF] = bit;
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case 0x9: {
            // Conditional skip
            if (v[X] != v[Y]) {
                pc += 2;
            }
            break;
        }
        case 0xA: {
            // Set I
            I = NNN;
            break;
        }
        case 0xB: {
            if (config.add_vx) {
                uint16_t XNN = (X << 8) | NN;
                pc = XNN + v[X];
            }
            else {
                pc = NNN + v[0];
            }
            break;
        }
        case 0xC: {
            // Generate random number & NN
            v[X] = random_int() & NN;
            break;
        }
        case 0xD: {
            // Draw N lines. Start reading from memory[I], and start
            // writing from row Y, col X

            // Skip if display wait and rendered this frame
		    if (config.display_wait && rendered) {
				pc -= 2; // rollback
				break;
		    }

            uint8_t startCol = v[X] % cols, startRow = v[Y] % rows, height = N;
            v[0xF] = 0;
            for (uint8_t i = 0; i < height; i++) {
                uint8_t row = startRow + i;
                if (row >= rows) break;
                uint8_t data = memory[I + i];
                for (int j = 0; j < 8; j++) {
                    uint8_t col = startCol + j;
                    if (col >= cols) break;
                    uint8_t pixel = (data >> (7 - j)) & 1;
                    if (pixel && display[row * cols + col]) v[0xF] = 1;
                    display[row * cols + col] ^= pixel;
                }
            }

            display_changed = true;
            rendered = true;
            break;
        }
        case 0xE: {
            switch (NN) {
                case 0x9E: {
                    // Skip if held
                    if (keys_held[v[X]]) pc += 2;
                    break;
                }
                case 0xA1: {
                    // Skip if not held
                    if (!keys_held[v[X]]) pc += 2;
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case 0xF: {
            switch (NN) {
                case 0x07: {
                    v[X] = delay_timer;
                    break;
                }
                case 0x15: {
                    delay_timer = v[X];
                    break;
                }
                case 0x18: {
                    sound_timer = v[X];
                    break;
                }
                case 0x1E: {
                    // Add to I

                    uint16_t sum = I + v[X];
                    v[0xF] = sum > 0x0FFF; // NOTE: might need to add to config
                    I = sum & 0x0FFF;
                    break;
                }
                case 0x0A: {
                    // Get key
                    bool found = false;
                    for (uint8_t i = 0; i < 16; i++) {
                        if (keys_released[i]) {
                            v[X] = i;
                            found = true;
                            keys_released[i] = false; // In case the instruction runs again in the same tick
                            break;
                        }
                    }
                    if (!found) pc -= 2;
                    break;
                }
                case 0x29: {
                    // Point to first byte of letter
                    I = font_start + letter_height * v[X];
                    break;
                }
                case 0x33: {
                    // Get BCD
                    uint8_t num = v[X];
                    memory[I] = num / 100;
                    memory[I + 1] = (num / 10) % 10;
                    memory[I + 2] = num % 10;
                    break;
                }
                case 0x55: {
                    // Load v into memory
                    for (uint8_t i = 0; i <= X; i++) {
                        memory[I + i] = v[i];
                    }
                    if (config.set_I) I = I + X + 1;
                    break;
                }
                case 0x65: {
                    // Get v from memory
                    for (uint8_t i = 0; i <= X; i++) {
                        v[i] = memory[I + i];
                    }
                    if (config.set_I) I = I + X + 1;
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
}