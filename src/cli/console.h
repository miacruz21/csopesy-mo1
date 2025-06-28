#pragma once
#include <string>
#include <vector>
#include <memory>
#include "core/process_manager.h"
#include "core/config_manager.h"

class ProcessManager;
class ConfigManager;

class Console {
public:
    Console();
    void run();
    void stop();
    
private:
    static const std::string HEADER;
    std::unique_ptr<ProcessManager> process_manager;
    ConfigManager config_manager;
    bool initialized = false;
    bool in_process_screen = false;
    bool exit_requested     = false;
    std::string current_process_name;
    bool enable_colors = true;

    // Main menu
    void print_header() const;
    void print_prompt() const;
    void show_help() const;

    // Utility for colors and clearing
    std::string get_color_green() const;
    std::string get_color_yellow() const;
    std::string get_color_reset() const;
    std::string get_color_red() const;
    std::string get_color_blue() const;
    bool colors_enabled() const;
    void clear_screen() const;

    // Handlers
    void handle_initialize();
    void handle_screen_command(const std::string& command);
    void handle_report_command();
    void handle_scheduler_start();
    void handle_scheduler_stop();
    void handle_process_command(const std::string& input);
    void enter_process_screen(const std::string& process_name);
    void exit_process_screen();

    // Helpers
    std::vector<std::string> split(const std::string& s);
    void handle_report_util();

};
