#include "cli/console.h"
#include <atomic>
#include <iostream>

std::atomic<uint64_t> cpu_cycles_counter{0};

int main() {
    try {
        Console console;
        console.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }
}