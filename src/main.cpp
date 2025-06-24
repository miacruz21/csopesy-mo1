#include "cli/console.hpp"
#include <iostream>

int main() {
    try {
        Console console;
        console.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}