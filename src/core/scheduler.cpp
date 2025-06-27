#include "scheduler.h"

// FCFS
void FCFSScheduler::add_process(std::shared_ptr<Process> p) { std::lock_guard<std::mutex> lk(mtx); q.push_back(p); }
std::shared_ptr<Process> FCFSScheduler::next_process() { std::lock_guard<std::mutex> lk(mtx); if (q.empty()) return nullptr; auto p = q.front(); q.pop_front(); return p; }
bool FCFSScheduler::has_processes() const { std::lock_guard<std::mutex> lk(mtx); return !q.empty(); }
void FCFSScheduler::reset() { std::lock_guard<std::mutex> lk(mtx); q.clear(); }

// Round Robin
RRScheduler::RRScheduler(uint64_t q) : quantum(q) {}
void RRScheduler::add_process(std::shared_ptr<Process> p) { std::lock_guard<std::mutex> lk(mtx); q.push_back(p); }
std::shared_ptr<Process> RRScheduler::next_process() { 
    std::lock_guard<std::mutex> lk(mtx);
    if (q.empty()) return nullptr;
    auto p = q.front(); q.pop_front();
    q.push_back(p);
    return p;
}
bool RRScheduler::has_processes() const { std::lock_guard<std::mutex> lk(mtx); return !q.empty(); }
void RRScheduler::reset() { std::lock_guard<std::mutex> lk(mtx); q.clear(); }