#pragma once
#include <string>
#include <chrono>
#include <ctime>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include <memory>
#include <iomanip>
#include "instruction.hpp"

class Instruction;

class Process {
private:
    std::string name;
    std::time_t created_time;
    std::time_t finished_time;
    int current_instruction = 0;
    int total_instructions;
    bool finished = false;
    int core_id = -1;
    
    std::vector<std::string> logs;
    std::unordered_map<std::string, uint16_t> variables;
    std::vector<std::unique_ptr<Instruction>> instructions;
    
    mutable std::mutex mutex;
    std::ofstream log_file;

    void add_log(const std::string& message);

    std::string format_time(std::time_t time) const;

public:
    Process(const std::string& name, int total_instructions);
    ~Process();

    uint16_t clamp_value(int32_t value) const;
    void execute();
    void attach_to_core(int core_id);
    void detach_from_core();

    void set_variable(const std::string& name, uint16_t value);
    uint16_t get_variable(const std::string& name) const;
    bool has_variable(const std::string& name) const;
    
    void add_instruction(std::unique_ptr<Instruction> instruction);
    void execute_next_instruction();
    void generate_random_instructions();
    void print_short_status() const;

    std::string get_name() const;
    bool is_finished() const;
    int get_core_id() const;
    int get_progress() const;
    std::time_t get_created_time() const;
    std::time_t get_finished_time() const;
    int get_current_instruction() const;
    int get_total_instructions() const;
    
    void print_status() const;
    void print_full_info() const;
    void print(const std::string& message);

    uint16_t get_variable_or_value(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex);
        if (variables.find(name) != variables.end()) {
            return variables.at(name);
        }
        try {
            return static_cast<uint16_t>(std::stoul(name));
        } catch (...) {
            return 0;
        }
    }
};
