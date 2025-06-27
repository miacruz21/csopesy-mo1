#include "cpu_utilization.h"
#include <iostream>
#include <fstream>

extern std::atomic<uint64_t> cpu_cycles_counter;

CPUUtilization::CPUUtilization(uint32_t total_cores)
    : total_cores_(total_cores),
      start_times(total_cores),
      busy_times(total_cores),
      t0(std::chrono::steady_clock::now())
{}

void CPUUtilization::mark_busy(int core) {
    std::lock_guard<std::mutex> lk(mtx);
    start_times[core] = std::chrono::steady_clock::now();
}

void CPUUtilization::mark_idle(int core) {
    auto t1 = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lk(mtx);
    busy_times[core] += t1 - start_times[core];
}

int CPUUtilization::get_busy_cores() const {
    std::lock_guard<std::mutex> lk(mtx);
    int c = 0;
    for (auto& st : start_times) if (st.time_since_epoch().count()) ++c;
    return c;
}

int CPUUtilization::get_available_cores() const {
    return cores - get_busy_cores();
}

double CPUUtilization::get_utilization_percent() const {
    int used = get_busy_cores();
    if (cores == 0) return 0.0;
    return 100.0 * used / cores;
}

void CPUUtilization::print_report() const {
    std::ofstream ofs("csopesy-log.txt");
    ofs << "CPU Utilization at cycle " << cpu_cycles_counter.load() << "\n";
    ofs << "Busy cores: " << get_busy_cores() << "\n";
    ofs << "Available cores: " << get_available_cores() << "\n";
}

void CPUUtilization::print_report(std::ostream& out) const {
    out << "CPU[" << cpu_cycles_counter.load() << "] "
        << get_busy_cores() << "/" << cores << " busy\n";
}