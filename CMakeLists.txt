cmake_minimum_required(VERSION 3.20)
project(chip8-emulator)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)

add_executable(chip8-emulator
        src/main.cpp
        src/chip8.cpp
)

target_include_directories(chip8-emulator PRIVATE include)
target_link_libraries(chip8-emulator PRIVATE SDL2::SDL2)
