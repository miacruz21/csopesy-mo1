#include "process.h"
#include "instruction.h"
#include <iostream>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cctype>

// Helper: formatted timestamp
std::string now_time() {
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%m/%d/%Y %I:%M:%S%p");
    return oss.str();
}

Process::Process(std::string name_, int id_, int min_ins, int max_ins, int delay)
    : name(std::move(name_)), id(id_) {
    created_time = now_time();
    std::mt19937 rng(static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count()) + id);
    std::uniform_int_distribution<int> count_dist(min_ins, max_ins);
    int num = count_dist(rng);

    std::vector<std::string> var_names;
    int var_count = 0;

    for (int i = 0; i < num; ++i) {
        if (i % 2 == 0) {
            code.push_back(std::make_unique<PrintInst>(
                "Step " + std::to_string(i + 1) + " of " + name
            ));
            continue;
        }
        int t = rng() % 4;
        if (t == 0) {
            std::string v = "v" + std::to_string(var_count++);
            int val = rng() % 100;
            var_names.push_back(v);
            code.push_back(std::make_unique<DeclareInst>(v, val));
        } else if (t == 1) {
            std::string dest = "v" + std::to_string(rng() % (var_count + 1));
            std::string op1 = (var_names.empty() ? "0" : var_names[rng() % var_names.size()]);
            std::string op2 = std::to_string(rng() % 50);
            code.push_back(std::make_unique<MathInst>(true, dest, op1, op2));
        } else if (t == 2) {
            std::string dest = "v" + std::to_string(rng() % (var_count + 1));
            std::string op1 = (var_names.empty() ? "0" : var_names[rng() % var_names.size()]);
            std::string op2 = std::to_string(rng() % 50);
            code.push_back(std::make_unique<MathInst>(false, dest, op1, op2));
        } else {
            int repeats = 1 + (rng() % 2);
            std::vector<std::unique_ptr<Instruction>> loop_body;
            int loop_len = 1 + (rng() % 2);
            for (int j = 0; j < loop_len; ++j) {
                loop_body.push_back(std::make_unique<SleepInst>(delay > 0 ? delay : 1));
            }
            code.push_back(std::make_unique<ForInst>(std::move(loop_body), repeats));
        }
    }
    // Guarantee at least one PRINT
    bool has_print = false;
    for (const auto& instr : code)
        if (dynamic_cast<PrintInst*>(instr.get())) { has_print = true; break; }
    if (!has_print) {
        code.insert(code.begin(), std::make_unique<PrintInst>("Auto: Hello from " + name));
    }
}

void Process::run_one_tick() {
    if (done) return;
    if (start_time.empty()) start_time = now_time();
    if (sleep_ticks > 0) { --sleep_ticks; return; }
    if (pc < code.size()) {
        code[pc++]->execute(*this);
    }
    if (pc >= code.size()) {
        done = true;
        if (finished_time.empty()) finished_time = now_time();
        if (!logs_saved) {
            save_logs_to_file();
            logs_saved = true;
        }
    }
}

void Process::log(const std::string& msg) {
    std::lock_guard<std::mutex> lk(mtx);
    logs.push_back(msg);

    // Append each log to a per-process file
    std::ofstream ofs("logs/" + name + ".txt", std::ios::app);
    ofs << msg << std::endl;
}

void Process::print_smi_info() const {
    std::cout << "===== Process Name: " << name << " =====\n";
    std::cout << "ID: " << id << "\n";
    std::cout << "Logs:\n";
    for (const auto& log : logs) {
        std::cout << log << std::endl;
    }
    std::cout << "\nCurrent instruction line: " << pc << std::endl;
    std::cout << "Lines of code: " << code.size() << std::endl;
    if (done) std::cout << "\nFINISHED!\n";
}

void Process::set_var(const std::string& var, int val) {
    if (val < 0) val = 0;
    if (val > 65535) val = 65535;
    vars[var] = val;
}

int Process::get_var_or_val(const std::string& s) const {
    if (isdigit(s[0])) return std::stoi(s);
    auto it = vars.find(s);
    return it != vars.end() ? it->second : 0;
}
void Process::sleep(int t) { sleep_ticks = t; }
bool Process::is_finished() const { return done; }

void Process::save_logs_to_file() const {
    std::filesystem::create_directory("logs");
    std::ofstream ofs("logs/" + name + ".txt");
    for (const auto& log : logs) ofs << log << std::endl;
    ofs << "Current instruction line: " << pc << std::endl;
    ofs << "Lines of code: " << code.size() << std::endl;
    if (done) ofs << "\nFINISHED\n";
}