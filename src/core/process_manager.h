#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include "config_manager.h"
#include "cpu_utilization.h"
#include "scheduler.h"
#include "process.h"

class ProcessManager {
public:
    ProcessManager(uint32_t cores);
    ~ProcessManager();

    void set_config_manager(ConfigManager *c);

    void initialize_scheduler(const std::string &alg, uint64_t quantum);
    void start_scheduler();
    void stop_scheduler();

    void start_batch_processing();
    void stop_batch_processing();

    std::shared_ptr<Process> get_process(const std::string &name) const;
    void add_process(const std::string &name);
    std::shared_ptr<Process> get_or_create_process(const std::string &name);

    void print_system_status(std::ostream& out) const;
    void print_process_lists(std::ostream&) const; 
    void generate_utilization_report() const;
    void shutdown();   
private:
    mutable std::mutex procs_mutex;
    std::vector<std::shared_ptr<Process>> procs;
    std::unique_ptr<SchedulerBase> sched;
    uint64_t rr_quantum_cycles_ = 1;
    bool scheduler_is_rr_   = false;
    ConfigManager* cfg = nullptr;
    CPUUtilization util;
    std::atomic<bool> running = false;
    std::atomic<bool> batching = false;
    std::thread sched_thread;
    std::thread batch_thread;
    std::atomic<uint64_t> next_id = 1;
};