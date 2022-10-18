#include <chrono>
#include <string>
#include <iostream>

template <typename F>
auto benchmark(F&& func) {
    auto start = std::chrono::system_clock::now();
    func();
    auto end = std::chrono::system_clock::now();
    return end - start;
}
