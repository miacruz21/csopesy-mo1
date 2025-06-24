#include "core/process.hpp"
#include "core/instruction.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <random>

Process::Process(const std::string& name, int total_instructions) 
    : name(name), total_instructions(total_instructions) {
    created_time = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    log_file.open(name + "_log.txt");
    add_log("Process created");
}

Process::~Process() {
    if (log_file.is_open()) {
        log_file.close();
    }
}

void Process::set_variable(const std::string& name, uint16_t value) {
    std::lock_guard<std::mutex> lock(mutex);
    variables[name] = value;
}

uint16_t Process::get_variable(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex);
    return variables.at(name);
}

bool Process::has_variable(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex);
    return variables.find(name) != variables.end();
}

void Process::add_log(const std::string& message) {
    std::time_t now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    
    std::lock_guard<std::mutex> lock(mutex);
    std::string log_entry = std::string(time_str) + " [" + name + "] " + message;
    logs.push_back(log_entry);
    if (log_file.is_open()) {
        log_file << log_entry << "\n";
    }
}

uint16_t Process::clamp_value(int32_t value) const {
    if (value < 0) return 0;
    if (value > UINT16_MAX) return UINT16_MAX;
    return static_cast<uint16_t>(value);
}

void Process::attach_to_core(int core_id) {
    std::lock_guard<std::mutex> lock(mutex);
    this->core_id = core_id;
    add_log("Attached to core " + std::to_string(core_id));
}

void Process::detach_from_core() {
    std::lock_guard<std::mutex> lock(mutex);
    add_log("Detached from core " + std::to_string(core_id));
    this->core_id = -1;
}

void Process::add_instruction(std::unique_ptr<Instruction> instruction) {
    std::lock_guard<std::mutex> lock(mutex);
    instructions.push_back(std::move(instruction));
}

void Process::execute_next_instruction() {
    std::lock_guard<std::mutex> lock(mutex);
    if (finished || current_instruction >= instructions.size()) {
        finished = true;
        finished_time = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        add_log("Process completed");
        return;
    }
    
    try {
        instructions[current_instruction]->execute(*this);
        add_log("Executed instruction: " + 
               instructions[current_instruction]->to_string());
        current_instruction++;
    } catch (const std::exception& e) {
        add_log("Error executing instruction: " + std::string(e.what()));
    }
    
    if (current_instruction >= total_instructions) {
        finished = true;
        finished_time = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        add_log("Process completed");
    }
}

std::string Process::get_name() const {
    std::lock_guard<std::mutex> lock(mutex);
    return name;
}

bool Process::is_finished() const {
    std::lock_guard<std::mutex> lock(mutex);
    return finished;
}

int Process::get_core_id() const {
    std::lock_guard<std::mutex> lock(mutex);
    return core_id;
}

int Process::get_progress() const {
    std::lock_guard<std::mutex> lock(mutex);
    return current_instruction;
}

std::time_t Process::get_created_time() const {
    return created_time;
}

std::time_t Process::get_finished_time() const {
    std::lock_guard<std::mutex> lock(mutex);
    return finished_time;
}

int Process::get_current_instruction() const {
    std::lock_guard<std::mutex> lock(mutex);
    return current_instruction;
}

int Process::get_total_instructions() const {
    return total_instructions;
}

void Process::print(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex);
    std::string output_file = name + "_output.txt";
    std::ofstream out(output_file, std::ios::app);
    if (out.is_open()) {
        out << message << "\n";
    }
    add_log("Printed: " + message);
}

void Process::print_status() const {
    std::lock_guard<std::mutex> lock(mutex);
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "%m/%d/%Y %I:%M:%S%p", std::localtime(&created_time));
    
    std::cout << std::setw(12) << std::left << name
              << std::setw(22) << time_str
              << "Core: " << std::setw(2) << core_id << "  "
              << std::setw(4) << current_instruction << "/" 
              << std::setw(4) << total_instructions;
    
    if (finished) std::cout << " [FINISHED]";
    else if (core_id == -1) std::cout << " [WAITING]";
    else std::cout << " [RUNNING]";
    
    std::cout << "\n";
}

void Process::print_short_status() const {
    std::lock_guard<std::mutex> lock(mutex);
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "%m/%d/%Y %I:%M:%S%p", std::localtime(&created_time));
    
    std::cout << std::setw(12) << std::left << name
              << " (" << format_time(created_time) << ") ";
    
    if (finished) {
        std::cout << "Finished   ";
    } else {
        std::cout << "Core: " << std::setw(2) << core_id << "   ";
    }
    
    std::cout << std::setw(4) << current_instruction << " / " 
              << std::setw(4) << total_instructions << "\n";
}

std::string Process::format_time(std::time_t time) const {
    char buf[100];
    std::strftime(buf, sizeof(buf), "%m/%d/%Y %I:%M:%S%p", std::localtime(&time));
    return std::string(buf);
}

void Process::print_full_info() const {
    std::lock_guard<std::mutex> lock(mutex);
    char created[100], finished[100];
    std::strftime(created, sizeof(created), "%m/%d/%Y %I:%M:%S%p", std::localtime(&created_time));
    
    std::cout << "\nProcess name: " << name << "\n";
    std::cout << "ID: " << std::hash<std::string>{}(name) % 10000 << "\n";
    std::cout << "Logs:\n";
    
    // Print last 5 logs or all if less than 5
    size_t start = logs.size() > 5 ? logs.size() - 5 : 0;
    for (size_t i = start; i < logs.size(); ++i) {
        std::cout << "(" << logs[i] << ")\n";
    }
    
    std::cout << "\nCurrent instruction line: " << current_instruction << "\n";
    std::cout << "Lines of code: " << total_instructions << "\n";
    
    if (finished) {
        std::strftime(finished, sizeof(finished), "%m/%d/%Y %I:%M:%S%p", std::localtime(&finished_time));
        std::cout << "\nFinished at: " << finished << "\n";
        std::cout << "Finished!\n";
    }
    std::cout << std::endl;
}


void Process::generate_random_instructions() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> type_dist(0, 5);
    std::uniform_int_distribution<> var_dist(0, 9);
    std::uniform_int_distribution<> value_dist(0, UINT16_MAX);
    std::uniform_int_distribution<> sleep_dist(1, 10);
    std::uniform_int_distribution<> loop_dist(1, 5);
    std::uniform_int_distribution<> inner_loop_dist(1, 3);

    for (int i = 0; i < total_instructions; ) {
        int instruction_type = type_dist(gen);
        std::string var_prefix = "var";
        
        try {
            switch (instruction_type) {
                case 0: { // PRINT
                    std::string var = var_prefix + std::to_string(var_dist(gen));
                    std::string msg = "Hello world from " + name + "!";
                    add_instruction(std::make_unique<PrintInstruction>(msg));
                    i++;
                    break;
                }
                case 1: { // DECLARE
                    std::string var = var_prefix + std::to_string(var_dist(gen));
                    uint16_t value = value_dist(gen);
                    add_instruction(std::make_unique<DeclareInstruction>(var, value));
                    i++;
                    break;
                }
                case 2: { // ADD
                    std::string var1 = var_prefix + std::to_string(var_dist(gen));
                    std::string var2 = var_prefix + std::to_string(var_dist(gen));
                    std::string var3 = var_prefix + std::to_string(var_dist(gen));
                    add_instruction(std::make_unique<AddInstruction>(var1, var2, var3));
                    i++;
                    break;
                }
                case 3: { // SUBTRACT
                    std::string var1 = var_prefix + std::to_string(var_dist(gen));
                    std::string var2 = var_prefix + std::to_string(var_dist(gen));
                    std::string var3 = var_prefix + std::to_string(var_dist(gen));
                    add_instruction(std::make_unique<SubtractInstruction>(var1, var2, var3));
                    i++;
                    break;
                }
                case 4: { // SLEEP
                    uint8_t ticks = sleep_dist(gen);
                    add_instruction(std::make_unique<SleepInstruction>(ticks));
                    i++;
                    break;
                }
                case 5: { // FOR LOOP
                    uint16_t repeats = loop_dist(gen);
                    std::vector<std::unique_ptr<Instruction>> loop_instructions;
                    
                    // Generate 1-3 instructions for the loop
                    int loop_size = inner_loop_dist(gen);
                    for (int j = 0; j < loop_size; j++) {
                        // Simplified - could recursively generate more complex structures
                        std::string var = var_prefix + std::to_string(var_dist(gen));
                        loop_instructions.push_back(
                            std::make_unique<PrintInstruction>("Loop iteration " + var)
                        );
                    }
                    
                    add_instruction(std::make_unique<ForLoopInstruction>(std::move(loop_instructions), repeats));
                    i++;
                    break;
                }
            }
        } catch (...) {
            continue;
        }
    }
}