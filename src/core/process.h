#pragma once
#include "instruction.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include <fstream>
#include <atomic>

class Process {
    std::string name;
    int id;
    std::vector<std::unique_ptr<Instruction>> code;
    size_t pc = 0;
    int sleep_ticks = 0;
    std::map<std::string, int> vars;
    std::vector<std::string> logs;
    mutable std::mutex mtx;
    bool done = false;
    bool logs_saved = false;
    std::string created_time;
    std::string start_time;
    std::string finished_time;
    std::ofstream log_stream; 
    std::atomic<int> core_id{-1};
public:
    Process() = default;
    Process(std::string name, int id, int min_ins, int max_ins, int delay);
    void run_one_tick();
    void log(const std::string& msg);
    void print_smi_info() const;
    void set_var(const std::string& var, int val);
    int get_var_or_val(const std::string& s) const;
    void sleep(int t);
    bool is_finished() const;
    int get_id() const { return id; }
    std::string get_name() const { return name; }
    size_t get_pc() const { return pc; }
    size_t get_code_size() const { return code.size(); }
    int get_core_id() const { return core_id; }
    const std::vector<std::string>& get_logs() const { return logs; }
    std::string get_created_time() const { return created_time; }
    std::string get_start_time() const { return start_time; }
    std::string get_finished_time() const { return finished_time; }
    size_t code_size()    const { return code.size(); }
    std::vector<std::string> recent_logs(size_t n) const;
    void set_core_id(int id) { core_id = id; }
};
