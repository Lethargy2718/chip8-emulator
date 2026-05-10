cmake_minimum_required(VERSION 3.20)
project(chip8-emulator)

set(CMAKE_CXX_STANDARD 20)

add_executable(chip8-emulator
        src/main.cpp
)
