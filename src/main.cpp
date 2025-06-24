#include "cli/console.hpp"
#include <iostream>

void enable_virtual_terminal_processing();

int main() {
    enable_virtual_terminal_processing();
    try {
        Console console;
        console.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}