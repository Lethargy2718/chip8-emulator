#pragma once

#include <cstdint>
#include <chrono>

struct chip8_config {
    bool set_vx = true;         // 0x8: set v[X] = v[Y] before shift.
    bool add_vx = false;        // 0xB: add V[X] before jump.
    bool set_I = true;          // 0xFX: set I after iteration ends (memory)
    bool vf_reset = true;       // 0x1, 0x2, 0x3: VF = 0 after bitwise ops
    bool display_wait = true;   // 0xD: render only once per 60Hz tick (1/60s)

    static chip8_config chip8() {
        return {}; // defaults are already CHIP-8
    }

    static chip8_config superchip_legacy() {
        return { .set_vx = false, .add_vx = true, .set_I = false, .vf_reset = false, .display_wait = true };
    }

    static chip8_config superchip_modern() {
        auto cfg = superchip_legacy();
        cfg.display_wait = false;
        return cfg;
    }

    // NOTE: always fails display_wait and clipping anyway since I haven't implemented hires
    static chip8_config xochip() {
        return { .set_vx = true, .add_vx = false, .set_I = true, .vf_reset = false, .display_wait = true };
    }
};

class chip8 {
public:
    explicit chip8(const chip8_config& config = {}) : config(config) {}

    static constexpr uint8_t cols = 64;
    static constexpr uint8_t rows = 32;

    bool keys_held[16]{};
    bool keys_released[16]{};

    void start(const std::string& filename);
    void tick();
    bool is_playing_sound() const { return sound_timer > 0; }

    const bool* get_display() const { return display; }
    bool should_display() const { return display_changed; }
    void reset_should_display() { display_changed = false; }
    void reset_keys_released() { memset(keys_released, 0, sizeof(keys_released)); }

private:
    chip8_config config;
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
    static constexpr uint8_t letter_height = 5;
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
    bool display[cols * rows]{}; // Black and white
    bool display_changed = true;
    bool rendered = false;

    // Stack
    uint16_t stack[16]{};
    uint8_t sp = 0; // 0 to 15. Points at the next element to write at

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

    void push(uint16_t i);

    uint16_t pop();
};
