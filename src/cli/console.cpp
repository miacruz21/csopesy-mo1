#include "console.h"
#include "core/process_manager.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif

std::vector<std::string> Console::split(const std::string &s)
{
    std::istringstream iss(s);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok)
        tokens.push_back(tok);
    return tokens;
}

Console::Console() : process_manager(nullptr) {}

std::string Console::get_color_green() const
{
    return colors_enabled() ? "\033[32m" : "";
}
std::string Console::get_color_yellow() const
{
    return colors_enabled() ? "\033[93m" : "";
}
std::string Console::get_color_reset() const
{
    return colors_enabled() ? "\033[0m" : "";
}
std::string Console::get_color_red() const
{
    return colors_enabled() ? "\033[31m" : "";
}
std::string Console::get_color_blue() const
{
    return colors_enabled() ? "\033[34m" : "";
}

bool Console::colors_enabled() const
{
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE)
    {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode))
        {
            return (dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
        }
    }
    return false;
#else
    return enable_colors;
#endif
}

void Console::clear_screen() const
{
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[1;1H";
#endif
}

const std::string Console::HEADER = R"(
    _____  _____  ____  _____  ______  _______    __
   / ____|/ ____|/ __ \|  __ \|  ____|/ ____|\ \  / /
  | |    | (___ | |  | | |__) | |__  | (___  \ \_/ /
  | |     \___ \| |  | |  ___/|  __|  \___ \  \  /
  | |____ ____) | |__| | |    | |____ ____) |  | |
   \_____|_____/ \____/|_|    |______|_____/   |_|
)";

void Console::print_header() const
{
    std::cout << get_color_reset();
    std::cout << get_color_green() << HEADER << get_color_reset() << std::endl;
    std::cout << get_color_yellow() << "Welcome to CSOPESY CPU Scheduler Emulator!" << get_color_reset() << std::endl;
    std::cout << "Type 'help' for available commands or 'initialize' to begin." << std::endl;
}

void Console::print_prompt() const {
    if (in_process_screen)
        std::cout << "[PROCESS]> ";
    else
        std::cout << "[MAIN]> ";
    std::cout.flush();
}

void Console::show_help() const
{
    if (in_process_screen)
    {
        std::cout << "Available commands:\n";
        std::cout << "    process-smi - Show process info and logs.\n";
        std::cout << "    exit        - Return to the main menu.\n";
    }
    else
    {
        std::cout << get_color_yellow() << "Available commands:\n"
                  << get_color_reset();
        std::cout << "    initialize          - Initialize the system from config.txt (must be run first).\n";
        std::cout << "    screen -s <name>    - Create a new process and attach to its screen.\n";
        std::cout << "    screen -ls          - List all running and finished processes.\n";
        std::cout << "    screen -r <name>    - Re-attach to a running process's screen.\n";
        std::cout << "    scheduler-start     - Start automatically generating dummy processes.\n";
        std::cout << "    scheduler-stop      - Stop generating dummy processes.\n";
        std::cout << "    report-util         - Generate and save CPU utilization report to csopesy-log.txt.\n";
        std::cout << "    exit                - Terminate the console.\n";
        std::cout << "    help                - Show this help message.\n";
        std::cout << "    clear               - Clear the console screen.\n";
    }
    std::cout.flush();
}

void Console::handle_initialize()
{
    try
    {
        std::ifstream test("config.txt");
        if (!test)
            throw std::runtime_error("config.txt not found in the current directory");
        test.close();

        config_manager.load("config.txt");

        int num_cpu = static_cast<int>(config_manager.get_long("num-cpu"));
        std::string scheduler_type = config_manager.get("scheduler");
        scheduler_type.erase(scheduler_type.find_last_not_of(" \t\n\r\"") + 1);
        scheduler_type.erase(0, scheduler_type.find_first_not_of(" \t\n\r\""));

        long quantum_cycles = config_manager.get_long("quantum-cycles");
        long batch_process_freq = config_manager.get_long("batch-process-freq");
        long min_ins = config_manager.get_long("min-ins");
        long max_ins = config_manager.get_long("max-ins");
        long delays_per_exec = config_manager.get_long("delays-per-exec");

        process_manager = std::make_unique<ProcessManager>(num_cpu);
        process_manager->initialize_scheduler(scheduler_type, static_cast<int>(quantum_cycles));
        process_manager->set_config_manager(&config_manager);
        initialized = true;
        process_manager->start_scheduler(); // <-- ADD THIS LINE
        std::cout << "System initialized successfully.\n";
    } catch (...) {
        initialized = false;
        throw;
    }
}

void Console::handle_screen_command(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd_root, sub_cmd, process_name;
    iss >> cmd_root >> sub_cmd;
    if ((sub_cmd == "-s" || sub_cmd == "-r")) {
        std::getline(iss >> std::ws, process_name);
        process_name.erase(0, process_name.find_first_not_of(" \t"));
        process_name.erase(process_name.find_last_not_of(" \t") + 1);
    }
    if (sub_cmd == "-s") {
        if (process_name.empty()) {
            std::cout << "Usage: screen -s <process_name>\n";
            return;
        }
        try {
            process_manager->add_process(process_name);
            auto process = process_manager->get_process(process_name);
            if (process && process->is_finished()) {
                std::cout << "Process " << process_name << " not found.\n";
                return;
            }
            in_process_screen = true;
            current_process_name = process_name;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    } else if (sub_cmd == "-ls") {
        process_manager->print_system_status();
    } else if (sub_cmd == "-r") {
        if (process_name.empty()) {
            std::cout << "Usage: screen -r <process_name>\n";
            return;
        }
        auto process = process_manager->get_process(process_name);
        if (process && !process->is_finished()) {
            in_process_screen = true;
            current_process_name = process_name;
        } else {
            std::cout << "Process " << process_name << " not found.\n";
        }
    } else {
        std::cout << "Invalid screen command. Use 'screen -s <name>', 'screen -r <name>', or 'screen -ls'.\n";
    }
}

void Console::enter_process_screen(const std::string& process_name) {
    in_process_screen = true;
    current_process_name = process_name;
    clear_screen();

    auto process = process_manager->get_process(process_name);
    if (!process) {
        std::cout << "Process " << process_name << " not found.\n";
        in_process_screen = false;
        return;
    }
    if (process->is_finished()) {
        std::cout << "Process " << process_name << " has finished execution.\n";
        in_process_screen = false;
        return;
    }

    while (in_process_screen) {
        process->print_smi_info();
        print_prompt(); // prints [PROCESS]>
        std::string input;
        if (!std::getline(std::cin, input)) break;

        std::string cmd = input;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

        if (cmd == "exit") {
            in_process_screen = false;
            clear_screen();
            print_header(); // restores main menu art and prompt
            break;
        } else if (cmd == "process-smi") {
            clear_screen(); // next loop will reprint
        } else {
            std::cout << "Invalid command. Use 'process-smi' or 'exit'.\n";
        }
    }
}

void Console::exit_process_screen() {
    in_process_screen = false;
    current_process_name.clear();
    clear_screen();
    print_header();
}

void Console::handle_report_command()
{
    process_manager->generate_utilization_report();
    std::cout << "Report generated and saved to csopesy-log.txt\n";
}

void Console::handle_scheduler_start()
{
    process_manager->start_batch_processing();
    std::cout << "Scheduler started generating dummy processes.\n";
}

void Console::handle_scheduler_stop()
{
    process_manager->stop_batch_processing();
    std::cout << "Scheduler stopped generating dummy processes.\n";
}

void Console::handle_process_command(const std::string &input)
{
    std::string cmd = input;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    auto process = process_manager->get_process(current_process_name);
    if (!process)
    {
        std::cout << "Process '" << current_process_name << "' no longer exists. Returning to main menu.\n";
        exit_process_screen();
        return;
    }
    if (cmd == "exit")
    {
        exit_process_screen();
        return;
    }
    if (cmd == "process-smi")
    {
        process->print_smi_info();
    }
    else
    {
        std::cout << "Invalid command. Use 'process-smi' or 'exit'.\n";
    }
}

void Console::run() {
    clear_screen();
    print_header();
    std::string input;

    while (true) {
        if (in_process_screen) {
            enter_process_screen(current_process_name);
            continue;
        }

        print_prompt(); // [MAIN]>
        if (!std::getline(std::cin, input)) break;

        // Clean whitespace
        input.erase(0, input.find_first_not_of(" \t\n\r"));
        input.erase(input.find_last_not_of(" \t\n\r") + 1);
        if (input.empty()) continue;

        std::string command_base = input.substr(0, input.find(' '));
        std::transform(command_base.begin(), command_base.end(), command_base.begin(), ::tolower);

        // ENFORCE: Only allow initialize and help if not initialized
        if ((!process_manager || !initialized) && command_base != "initialize" && command_base != "help") {
            std::cout << "System not initialized. Please run 'initialize' first.\n";
            continue;
        }
        if (command_base == "exit") break;
        if (command_base == "initialize") handle_initialize();
        else if (command_base == "help") show_help();
        else if (command_base == "clear") clear_screen();
        else if (command_base == "screen") handle_screen_command(input);
        else if (command_base == "scheduler-start" || command_base == "scheduler-test") handle_scheduler_start();
        else if (command_base == "scheduler-stop") handle_scheduler_stop();
        else if (command_base == "report-util") handle_report_command();
        else std::cout << "Invalid command. Type 'help' for available commands.\n";
    }
    std::cout << "Terminating console. Goodbye!\n";
    if (process_manager) {
        process_manager->stop_batch_processing();
        process_manager->stop_scheduler();
    }
}