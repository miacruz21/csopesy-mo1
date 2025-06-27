#pragma once
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>
#include <ostream>

class CPUUtilization {
    int cores;
    mutable std::mutex mtx;
    std::vector<std::chrono::steady_clock::time_point> start_times;
    std::vector<std::chrono::nanoseconds> busy_times;
    std::chrono::steady_clock::time_point t0;
    int total_cores_;
    int busy_cores_;
public:
    CPUUtilization(uint32_t total_cores);
    void mark_busy(int core);
    void mark_idle(int core);
    int get_busy_cores() const;
    int get_available_cores() const;
    void print_report() const;
    void print_report(std::ostream& out) const;
    double get_utilization_percent() const;
    int get_total_cores() const;
};