#pragma once
#include <string>
#include <memory>
#include "core/process_manager.hpp"

class Console {
private:
    std::unique_ptr<ProcessManager> process_manager;
    bool initialized = false;
    bool in_process_screen = false;
    std::string current_process;
    
    void clear_screen() const;
    void print_header() const;
    void show_help() const;
    void print_prompt() const;
    
    void handle_initialize();
    void handle_screen_command(const std::string& command);
    void handle_report_command();
    
    void enter_process_screen(const std::string& process_name);
    void exit_process_screen();
    void handle_process_command(const std::string& input);

    void handle_scheduler_start();
    void handle_scheduler_stop();
    void generate_utilization_report(bool to_file = false);

public:
    void run();
};