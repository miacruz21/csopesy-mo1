#pragma once
#include "process.hpp"
#include "scheduler.hpp"
#include "config_manager.hpp"
#include "cpu_utilization.hpp"
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <condition_variable>
#include <atomic>

class ProcessManager {
private:
    std::vector<std::shared_ptr<Process>> processes;
    std::unordered_map<std::string, std::shared_ptr<Process>> process_map;
    int total_cores;
    std::vector<std::thread> worker_threads;
    std::thread batch_thread;
    bool running = false;
    std::atomic<bool> batch_running{false};
    std::condition_variable cv;
    bool stop_requested = false;
    mutable std::mutex mutex;
    std::unique_ptr<Scheduler> scheduler;
    ConfigManager config;
    std::unique_ptr<CPUUtilization> cpu_util;
    std::atomic<int> process_counter{0};

    void worker_thread_func(int core_id);
    void scheduler_thread_func();
    void batch_process_generator();

    void add_process(const std::string &name, int total_instructions);

public:
    explicit ProcessManager(int cores = 4);
    ~ProcessManager();

    void initialize(const std::string& config_file);
    void start_scheduler();
    void stop_scheduler();
    void start_batch_processing();
    void stop_batch_processing();

    void add_process(const std::string& name);
    std::shared_ptr<Process> get_process(const std::string& name) const;
    bool all_processes_completed() const;
    void print_system_status() const;
    void generate_utilization_report() const;
    
    const ConfigManager& get_config() const { return config; }
};