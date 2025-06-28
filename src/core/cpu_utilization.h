#pragma once
#include <chrono>
#include <mutex>
#include <vector>
#include <ostream>

class CPUUtilization {
public:
    explicit  CPUUtilization(uint32_t cores);
    void      mark_busy(int core);
    void      mark_idle(int core);
    int       get_busy_cores()        const;
    int       get_available_cores()   const;
    double    get_utilization_percent() const;
    int       get_total_cores()       const;
    void      print_report(std::ostream&) const;
private:
    mutable std::mutex mtx;
    std::vector<std::chrono::steady_clock::time_point> start_times;
    std::vector<std::chrono::nanoseconds>             busy_times;
    std::vector<bool>                                 is_busy;
    std::chrono::steady_clock::time_point             t0;
    uint32_t                                          total_cores_;
    uint32_t                         busy_cores_{0};
};