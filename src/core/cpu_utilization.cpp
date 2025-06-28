#include "cpu_utilization.h"
#include <iostream>
#include <fstream>
#include <atomic>
#include <algorithm>
#include <iomanip>

std::atomic<uint64_t> cpu_cycles_counter{0};

CPUUtilization::CPUUtilization(uint32_t total_cores)
    : total_cores_(total_cores),
      start_times(total_cores),
      busy_times(total_cores),
      is_busy(total_cores,false),
      t0(std::chrono::steady_clock::now())
{}

void CPUUtilization::mark_busy(int core) {
    std::lock_guard<std::mutex> lk(mtx);
    if (!is_busy[core]) {                      
        start_times[core] = std::chrono::steady_clock::now();
        is_busy[core] = true;
    }
}

void CPUUtilization::mark_idle(int core) {
    auto t1 = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lk(mtx);
    if (is_busy[core]) {
        busy_times[core] += t1 - start_times[core];
        is_busy[core] = false;
    }
}

int CPUUtilization::get_busy_cores() const {
    std::lock_guard<std::mutex> lk(mtx);
    return std::count(is_busy.begin(), is_busy.end(), true);
}

int CPUUtilization::get_available_cores() const {
    return static_cast<int>(total_cores_) - get_busy_cores();
}

double CPUUtilization::get_utilization_percent() const {
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - t0).count();
    if (elapsed == 0) return 0.0;

    std::lock_guard<std::mutex> lk(mtx);
    std::chrono::nanoseconds busy{};
    for (size_t i = 0; i < total_cores_; ++i) {
        busy += busy_times[i];
        if (is_busy[i]) busy += now - start_times[i];
    }
    double busy_sec  = std::chrono::duration<double>(busy).count();
    double total_sec = elapsed * total_cores_;
    return (busy_sec / total_sec) * 100.0;
}

int CPUUtilization::get_total_cores() const {
    return static_cast<int>(total_cores_);
}

void CPUUtilization::print_report(std::ostream& out) const
{
    out << "CPU utilization: " << std::fixed << std::setprecision(1)
        << get_utilization_percent() << "%\n"
        << "Cores used: "      << get_busy_cores()      << '\n'
        << "Cores available: " << get_available_cores() << '\n';
}