#pragma once

#include <cstdint>
#include <chrono>

class chip8 {
public:
    static constexpr uint8_t display_width = 64;
    static constexpr uint8_t display_height = 32;

    bool keys[16]{};

    void start(const std::string& filename);
    void tick();
    bool is_playing_sound() const { return sound_timer > 0; }
    const bool* get_display() const { return display; }
    bool should_display() const { return display_changed; }
    void reset_should_display() { display_changed = false; }

private:
    // Address size = 16 bits (0x000 to 0xFFF)
    // Word size = 8 bits (0x00 to 0xFF)
    uint8_t memory[4096]{};
    uint16_t pc = 0x200;
    uint16_t I{};

    // General purpose registers
    uint8_t v[16]{};

    // Font
    static constexpr uint16_t font_start = 0x050;
    static constexpr uint8_t font_size = 80;
    static constexpr uint8_t font[font_size] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
        0x20, 0x60, 0x20, 0x20, 0x70,  // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
        0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
        0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
        0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
        0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
        0xF0, 0x80, 0xF0, 0x80, 0x80   // F
    };

    // Display
    bool display[display_width * display_height]{}; // Black and white
    bool display_changed = true;

    // Stack
    uint16_t stack[16]{};
    uint8_t sp = 0; // 0 to 15

    // Timing
    uint8_t delay_timer{};
    uint8_t sound_timer{};

    double last_frame = 0.0;
    double cpu_accumulator = 0.0;
    double timer_accumulator = 0.0;

    static constexpr double cpu_hz = 700.0;
    static constexpr double cpu_dt = 1.0 / cpu_hz;

    static constexpr double timer_dt = 1.0 / 60.0;

    /* Methods */

    bool load_rom(const std::string& filename);

    void initialize_font();

    void tick_timers();

    uint16_t fetch();

    void decode_and_execute(uint16_t instruction);
};
