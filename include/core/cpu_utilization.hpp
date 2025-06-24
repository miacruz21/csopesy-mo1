#pragma once
#include <vector>
#include <chrono>
#include <mutex>
#include <string>

class CPUUtilization {
private:
    std::vector<std::chrono::steady_clock::time_point> core_busy_times;
    std::vector<std::chrono::steady_clock::time_point> core_idle_times;
    std::vector<unsigned long> instructions_executed;
    int total_cores;
    mutable std::mutex mutex;
    int get_busy_cores() const;

    std::string get_current_time() const;

public:
    explicit CPUUtilization(int cores);
    void mark_busy(int core);
    void mark_idle(int core);
    void count_instruction(int core);
    double get_utilization(int core) const;
    void reset();
    void print_report() const;
};