#include "cli/console.hpp"
#include <iostream>
#include <algorithm>
#include <iomanip>
#ifdef _WIN32
#include <windows.h>
#endif

void Console::clear_screen() const {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void enable_virtual_terminal_processing() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

const std::string COLOR_GREEN = "\033[32m";
const std::string COLOR_LIGHT_YELLOW = "\033[93m";
const std::string COLOR_RESET = "\033[0m";
const std::string HEADER = R"(
   _____  _____  ____  _____  ______  _______     __
  / ____|/ ____|/ __ \|  __ \|  ____|/ ____\ \   / /
 | |    | (___ | |  | | |__) | |__  | (___  \ \_/ / 
 | |     \___ \| |  | |  ___/|  __|  \___ \  \   /  
 | |____ ____) | |__| | |    | |____ ____) |  | |   
  \_____|_____/ \____/|_|    |______|_____/   |_|   
)"
+ COLOR_GREEN + "\nWelcome to CSOPESY commandline!\n" + COLOR_RESET
+ COLOR_LIGHT_YELLOW + "Type 'initialize' to begin. 'exit' to quit, 'clear' to refresh\n" + COLOR_RESET;

void Console::print_header() const {
    std::cout << HEADER << "\n";
}

void Console::show_help() const {
    std::cout << "Available commands:\n"
              << "  initialize - Initialize system\n"
              << "  screen -s <name> - Create new process screen\n"
              << "  screen -r <name> - Attach to process screen\n"
              << "  screen -ls - List all processes\n"
              << "  scheduler-start - Start scheduler\n"
              << "  scheduler-stop - Stop scheduler\n"
              << "  report-util - Generate utilization report\n"
              << "  clear - Refresh console\n"
              << "  exit - Quit program\n";
}

void Console::handle_initialize() {
    try {
        process_manager = std::make_unique<ProcessManager>();
        process_manager->initialize("config.txt");
        initialized = true;
        std::cout << "System initialized successfully\n";
    } catch (const std::exception& e) {
        std::cerr << "Initialization failed: " << e.what() << "\n";
    }
}

void Console::enter_process_screen(const std::string& process_name) {
    auto process = process_manager->get_process(process_name);
    if (!process) {
        std::cout << "Process " << process_name << " not found\n";
        return;
    }
    
    clear_screen();
    std::cout << "=== Process Screen: " << process_name << " ===\n";
    std::cout << "Type 'process-smi' for status, 'exit' to return to main console\n\n";
    
    in_process_screen = true;
    current_process = process_name;
    process->print_full_info();
}

void Console::print_prompt() const {
    if (!in_process_screen) {
        std::cout << "> ";
    } else {
        std::cout << current_process << "\\> ";
    }
}

void Console::handle_screen_command(const std::string& command) {
    if (command == "screen -ls") {
        process_manager->print_system_status();
    } 
    else if (command.rfind("screen -s ", 0) == 0) {
        std::string name = command.substr(10);
        process_manager->add_process(name);
        enter_process_screen(name);
    }
    else if (command.rfind("screen -r ", 0) == 0) {
        std::string name = command.substr(10);
        auto process = process_manager->get_process(name);
        if (process && !process->is_finished()) {
            enter_process_screen(name);
        } else {
            std::cout << "Process " << name << " not found or already finished\n";
        }
    }
    else {
        std::cout << "Invalid screen command\n";
    }
}

void Console::handle_scheduler_start() {
    if (!initialized) {
        std::cout << "System not initialized\n";
        return;
    }
    process_manager->start_batch_processing();
    std::cout << "Started generating processes\n";
}

void Console::handle_scheduler_stop() {
    process_manager->stop_batch_processing();
    std::cout << "Stopped generating processes\n";
}

void Console::exit_process_screen() {
    in_process_screen = false;
    current_process.clear();
    clear_screen();
    print_header();
}

void Console::handle_process_command(const std::string& input) {
    if (input == "process-smi") {
        auto process = process_manager->get_process(current_process);
        if (process) {
            process->print_full_info();
            if (process->is_finished()) {
                std::cout << "\nFinished!\n";
            }
        } else {
            std::cout << "Process not found\n";
            exit_process_screen();
        }
    }
    else if (input == "exit") {
        exit_process_screen();
    }
    else {
        std::cout << "Unknown process command. Type 'process-smi' or 'exit'\n";
    }
}

void Console::run() {
    print_header();
    
    std::string input;
    while (true) {
        print_prompt();
        std::getline(std::cin, input);
        
        // Trim input
        input.erase(input.begin(), std::find_if(input.begin(), input.end(), [](int ch) { 
            return !std::isspace(ch); 
        }));
        input.erase(std::find_if(input.rbegin(), input.rend(), [](int ch) { 
            return !std::isspace(ch); 
        }).base(), input.end());
        
        if (input.empty()) continue;
        
        if (input == "exit") {
            if (in_process_screen) exit_process_screen();
            else break;
        }
        else if (!initialized && input != "initialize" && input != "help" && input != "clear") {
            std::cout << "System not initialized. Type 'initialize' first.\n";
            continue;
        }
        
        if (in_process_screen) {
            // Changed from handle_screen_command to handle_process_command
            handle_process_command(input);
        } else {
            std::transform(input.begin(), input.end(), input.begin(), ::tolower);
            
            if (input == "initialize") handle_initialize();
            else if (input == "clear") clear_screen();
            else if (input == "help") show_help();
            else if (input.rfind("screen", 0) == 0) handle_screen_command(input);
            else if (input == "scheduler-start") handle_scheduler_start();
            else if (input == "scheduler-stop") handle_scheduler_stop();
            else if (input == "report-util") generate_utilization_report(true);
            else std::cout << "Invalid command. Type 'help' for available commands.\n";
        }
    }
}

void Console::generate_utilization_report(bool to_file) {
    if (!initialized) {
        std::cout << "System not initialized\n";
        return;
    }
    if (to_file) {
        process_manager->generate_utilization_report();
        std::cout << "Report generated at csopesy-log.txt!\n";
    } else {
        process_manager->print_system_status();
    }
}