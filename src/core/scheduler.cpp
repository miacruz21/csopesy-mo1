#include "core/scheduler.hpp"

// FCFS
void FCFSScheduler::add_process(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push_back(process);
}

std::shared_ptr<Process> FCFSScheduler::next_process() {
    std::lock_guard<std::mutex> lock(mutex);
    if (queue.empty()) return nullptr;
    
    auto process = queue.front();
    queue.erase(queue.begin());
    return process;
}

bool FCFSScheduler::has_processes() const {
    std::lock_guard<std::mutex> lock(mutex);
    return !queue.empty();
}

void FCFSScheduler::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    queue.clear();
}

// Round Robin
RRScheduler::RRScheduler(int quantum) : quantum(quantum) {}

void RRScheduler::add_process(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push_back(process);
}

std::shared_ptr<Process> RRScheduler::next_process() {
    std::lock_guard<std::mutex> lock(mutex);
    if (queue.empty()) return nullptr;
    
    current_index = current_index % queue.size();
    auto process = queue[current_index];
    current_index = (current_index + 1) % queue.size();
    return process;
}

bool RRScheduler::has_processes() const {
    std::lock_guard<std::mutex> lock(mutex);
    return !queue.empty();
}

void RRScheduler::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    queue.clear();
    current_index = 0;
}