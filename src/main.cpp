#include "cli/console.h"
#include <atomic>
#include <iostream>
#include <csignal>

static std::unique_ptr<Console> global_console;

int main() {
    signal(SIGINT, [](int){ if (global_console) global_console->stop(); });

    try {
        global_console = std::make_unique<Console>();
        global_console->run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    }
}