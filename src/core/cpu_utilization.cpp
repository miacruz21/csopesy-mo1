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
      busy_cores_(0),
      t0(std::chrono::steady_clock::now())
{}

void CPUUtilization::mark_busy(int core)
{
    std::lock_guard<std::mutex> lk(mtx);
    if (!is_busy[core]) {          // first time this quantum
        is_busy[core] = true;
        ++busy_cores_;
    }
}

void CPUUtilization::mark_idle(int core)
{
    std::lock_guard<std::mutex> lk(mtx);
    if (is_busy[core]) {           // only once per quantum
        is_busy[core] = false;
        --busy_cores_;
    }
}

int CPUUtilization::get_busy_cores() const
{
    std::lock_guard<std::mutex> lk(mtx);
    return static_cast<int>(busy_cores_);
}

int CPUUtilization::get_available_cores() const
{
    std::lock_guard<std::mutex> lk(mtx);
    return static_cast<int>(total_cores_ - busy_cores_);
}

double CPUUtilization::get_utilization_percent() const
{
    std::lock_guard<std::mutex> lk(mtx);
    return total_cores_ == 0
         ? 0.0
         : (busy_cores_ * 100.0) / static_cast<double>(total_cores_);
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