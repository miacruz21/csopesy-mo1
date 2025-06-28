#include "process.h"
#include "instruction.h"
#include "time_utils.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <algorithm>
#include "config_manager.h"

Process::Process(std::string name_, int id_,
                 int min_ins, int max_ins, int delay)
    : name(std::move(name_)), id(id_)
{
    created_time = util::now_time();

    /* ── build random program ───────────────────────────── */
    std::mt19937 rng(
        static_cast<unsigned>(std::chrono::system_clock::now()
                                  .time_since_epoch()
                                  .count()) +
        id);

    std::uniform_int_distribution<int> icount(min_ins, max_ins);
    int N = icount(rng);

    std::vector<std::string> var_names;
    int var_count = 0;

    for (int i = 0; i < N; ++i) {
        if (i % 2 == 0) {                    // PRINT every other step
            code.push_back(
                std::make_unique<PrintInst>("Step " + std::to_string(i + 1) +
                                             " of " + name));
            continue;
        }
        int t = rng() % 4;
        if (t == 0) {                        // DECL
            std::string v = "v" + std::to_string(var_count++);
            int val       = rng() % 100;
            var_names.push_back(v);
            code.push_back(std::make_unique<DeclInst>(v, val));
        } else if (t == 1 || t == 2) {       // ADD / SUB
            std::string dest = "v" + std::to_string(rng() % (var_count + 1));
            std::string op1 =
                var_names.empty() ? "0"
                                  : var_names[rng() % var_names.size()];
            std::string op2 = std::to_string(rng() % 50);
            bool is_add     = (t == 1);
            code.push_back(
                std::make_unique<MathInst>(dest, op1, op2, is_add));
        } else {                             // FOR loop of SLEEPs
            int repeats = 1 + rng() % 2;
            int looplen = 1 + rng() % 2;
            std::vector<std::unique_ptr<Instruction>> body;
            for (int j = 0; j < looplen; ++j)
                body.push_back(
                    std::make_unique<SleepInst>(delay > 0 ? delay : 1));
            code.push_back(
                std::make_unique<ForInst>(repeats, std::move(body)));
        }
    }
    if (std::none_of(code.begin(), code.end(), [](auto &up) {
            return dynamic_cast<PrintInst *>(up.get()) != nullptr;
        })) {
        code.insert(code.begin(),
                    std::make_unique<PrintInst>("Auto: Hello from " + name));
    }

    std::filesystem::create_directories("logs");
    log_stream.open("logs/" + name + ".txt", std::ios::app);
}

void Process::run_one_tick() {
    if (done) return;
    if (start_time.empty()) start_time = util::now_time();
    if (sleep_ticks > 0) { --sleep_ticks; return; }

    if (pc < code.size()) {
        auto &inst = code[pc++];
        inst->execute(*this);

        /* ── ensure file is open, then write ── */
        if (!log_stream.is_open())
            log_stream.open("logs/" + name + ".txt", std::ios::app);

        log_stream << util::now_time()
                   << " PC=" << (pc - 1) << ' '
                   << inst->tag() << '\n';
    }

    if (pc >= code.size()) {
        done = true;
        if (finished_time.empty()) finished_time = util::now_time();
        if (!log_stream.is_open())
            log_stream.open("logs/" + name + ".txt", std::ios::app);
        log_stream << "FINISHED at " << finished_time << '\n';
    }
}


void Process::log(const std::string& msg) {
    std::lock_guard<std::mutex> lk(mtx);
    if (!log_stream.is_open())
        log_stream.open("logs/" + name + ".txt", std::ios::app);
    log_stream << msg << '\n';
}

void Process::print_smi_info() const
{
    std::cout << "===== Process Name: " << name << " =====\n"
              << "ID: " << id << '\n'
              << "Current PC: " << pc << '/' << code.size() << '\n';
    if (done) std::cout << "\nFINISHED!\n";
}

void Process::set_var(const std::string &var, int val)
{
    if (val < 0) val = 0;
    if (val > 65535) val = 65535;
    vars[var] = val;
}

int Process::get_var_or_val(const std::string &s) const
{
    if (isdigit(s[0])) return std::stoi(s);
    auto it = vars.find(s);
    return it != vars.end() ? it->second : 0;
}

/* simple accessors */
void Process::sleep(int t)   { sleep_ticks = t; }
bool Process::is_finished() const { return done; }