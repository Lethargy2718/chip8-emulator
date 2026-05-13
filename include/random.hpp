#pragma once

#include <random>

inline double random_double() {
    thread_local std::mt19937 generator(std::random_device{}());
    thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(generator);
}

inline int random_int() {
    return static_cast<int>(random_double());
}