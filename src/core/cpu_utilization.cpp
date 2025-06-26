#include "core/cpu_utilization.hpp"
#include <iomanip>
#include <iostream>
#include <ctime>
#include <fstream>
#include <numeric>

CPUUtilization::CPUUtilization(int cores) 
    : total_cores(cores),
      core_busy_times(cores),
      core_idle_times(cores),
      instructions_executed(cores, 0) {}

void CPUUtilization::mark_busy(int core) {
    std::lock_guard<std::mutex> lock(mutex);
    if (core >= 0 && core < total_cores) {
        core_busy_times[core] = std::chrono::steady_clock::now();
    }
}

void CPUUtilization::mark_idle(int core) {
    std::lock_guard<std::mutex> lock(mutex);
    if (core >= 0 && core < total_cores) {
        core_idle_times[core] = std::chrono::steady_clock::now();
    }
}

void CPUUtilization::count_instruction(int core) {
    std::lock_guard<std::mutex> lock(mutex);
    if (core >= 0 && core < total_cores) {
        instructions_executed[core]++;
    }
}

double CPUUtilization::get_utilization(int core) const {
    std::lock_guard<std::mutex> lock(mutex);
    if (core < 0 || core >= total_cores) return 0.0;
    
    auto now = std::chrono::steady_clock::now();
    auto busy_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - core_busy_times[core]).count();
    
    // Always report 100% if core is marked busy
    if (busy_time > 0) return 100.0;
    return 0.0;
}

void CPUUtilization::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    // Reset all timestamps to current time
    auto now = std::chrono::steady_clock::now();
    std::fill(core_busy_times.begin(), core_busy_times.end(), now);
    std::fill(core_idle_times.begin(), core_idle_times.end(), now);
    // Reset instruction counters
    std::fill(instructions_executed.begin(), instructions_executed.end(), 0);
}

int CPUUtilization::get_busy_cores() const {
    std::lock_guard<std::mutex> lock(mutex);
    int busy_cores = 0;
    for (int i = 0; i < total_cores; ++i) {
        if (get_utilization(i) > 0.0) {
            busy_cores++;
        }
    }
    return busy_cores;
}

std::string CPUUtilization::get_current_time() const {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
    return std::string(buf);
}

void CPUUtilization::print_report() const {
    std::lock_guard<std::mutex> lock(mutex);
    std::ofstream file("csopesy-log.txt");
    
    file << "CSOPESY Utilization Report\n";
    file << "=========================\n";
    file << "Generated: " << get_current_time() << "\n\n";
    
    // CPU Utilization
    double total_util = 0.0;
    file << "Core Utilization:\n";
    for (int i = 0; i < total_cores; ++i) {
        double util = get_utilization(i);
        total_util += util;
        file << "  Core " << i << ": " 
             << std::fixed << std::setprecision(2) 
             << util << "%"
             << " (Instructions: " << instructions_executed[i] << ")\n";
    }
    
    file << "\nTotal CPU Utilization: " << std::fixed << std::setprecision(2)
         << (total_util / total_cores) << "%\n";
    file << "Total Instructions Executed: " 
         << std::accumulate(instructions_executed.begin(), 
                           instructions_executed.end(), 0) << "\n";
}