#include "core/process_manager.hpp"
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <iostream>
#include <random>
#include "core/process.hpp"

ProcessManager::ProcessManager(int cores) 
    : total_cores(cores), 
      cpu_util(std::make_unique<CPUUtilization>(cores)) {}

ProcessManager::~ProcessManager() {
    stop_scheduler();
    stop_batch_processing();
    
    // Ensure all threads are joined
    for (auto& thread : worker_threads) {
        if (thread.joinable()) thread.join();
    }
    worker_threads.clear();
}

void ProcessManager::initialize(const std::string& config_file) {
    try {
        config.load(config_file);
        total_cores = config.get_int("num-cpu");
        if (total_cores < 1 || total_cores > 128) {
            throw std::runtime_error("Invalid number of CPU cores. Must be between 1 and 128");
        }
        cpu_util = std::make_unique<CPUUtilization>(total_cores);
        
        std::string scheduler_type = config.get("scheduler");
        std::transform(scheduler_type.begin(), scheduler_type.end(), scheduler_type.begin(), ::tolower);
        
        if (scheduler_type == "fcfs") {
            scheduler = std::make_unique<FCFSScheduler>();
        } else if (scheduler_type == "rr") {
            int quantum = config.get_int("quantum-cycles");
            if (quantum < 1) {
                throw std::runtime_error("Quantum cycles must be at least 1");
            }
            scheduler = std::make_unique<RRScheduler>(quantum);
        } else {
            throw std::runtime_error("Unknown scheduler type: " + scheduler_type);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Initialization failed: " + std::string(e.what()));
    }
}

void ProcessManager::worker_thread_func(int core_id) {
    while (running) {
        std::shared_ptr<Process> process;
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this]() {
                return stop_requested || scheduler->has_processes();
            });
            
            if (stop_requested) return;
            
            process = scheduler->next_process();
            if (!process) continue;
            
            process->attach_to_core(core_id);
            cpu_util->mark_busy(core_id);
        }
        
        // Execute process
        while (!process->is_finished() && !stop_requested) {
            process->execute_next_instruction();
            cpu_util->count_instruction(core_id);
            
            // Small delay to simulate work
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        {
            std::lock_guard<std::mutex> lock(mutex);
            process->detach_from_core();
            cpu_util->mark_idle(core_id);
            
            // Ensure process is marked finished
            if (!process->is_finished()) {
                process->execute_next_instruction(); // Force finish
            }
        }
    }
}

void ProcessManager::scheduler_thread_func() {
    while (running && !stop_requested) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            
            // Distribute processes to cores
            for (int i = 0; i < total_cores; i++) {
                if (scheduler->has_processes()) {
                    auto process = scheduler->next_process();
                    if (process && process->get_core_id() == -1) {
                        process->attach_to_core(i);
                        cpu_util->mark_busy(i);
                    }
                }
            }
        }
        
        // Small delay to prevent busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ProcessManager::start_scheduler() {
    if (running) return;
    
    running = true;
    stop_requested = false;
    
    // Start scheduler thread
    worker_threads.emplace_back(&ProcessManager::scheduler_thread_func, this);
    
    // Start worker threads (one per core)
    for (int i = 0; i < total_cores; ++i) {
        worker_threads.emplace_back(&ProcessManager::worker_thread_func, this, i);
    }
}

void ProcessManager::stop_scheduler() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!running) return;
        
        running = false;
        stop_requested = true;
    }
    cv.notify_all();
    
    for (auto& thread : worker_threads) {
        if (thread.joinable()) thread.join();
    }
    worker_threads.clear();
}

void ProcessManager::start_batch_processing() {
    if (batch_running) return;
    batch_running = true;
    batch_thread = std::thread(&ProcessManager::batch_process_generator, this);
}

void ProcessManager::stop_batch_processing() {
    if (!batch_running) return;
    batch_running = false;
    if (batch_thread.joinable()) {
        batch_thread.join();
    }
}

void ProcessManager::add_process(const std::string& name) {
    // Call your existing add_process with a default instruction count, e.g., 10
    add_process(name, 10);
}

void ProcessManager::add_process(const std::string& name, int total_instructions) {
    auto process = std::make_shared<Process>(name, total_instructions);
    process->generate_random_instructions();
    
    {
        std::lock_guard<std::mutex> lock(mutex);
        processes.push_back(process);
        process_map[name] = process;
        scheduler->add_process(process);
    }
    cv.notify_one();
}

void ProcessManager::batch_process_generator() {
    std::random_device rd;
    std::mt19937 gen(rd());
    int freq_ms = config.get_int("batch-process-freq") * 100;
    int counter = 1;
    
    while (batch_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(freq_ms));
        
        // Only add new process if we have available cores
        {
            std::lock_guard<std::mutex> lock(mutex);
            int running_count = std::count_if(processes.begin(), processes.end(),
                [](const auto& p) { return !p->is_finished(); });
            
            if (running_count >= total_cores) {
                continue;  // Skip if all cores are busy
            }
        }
        
        std::string name = "p" + std::to_string(counter++);
        int min_ins = config.get_int("min-ins");
        int max_ins = config.get_int("max-ins");
        std::uniform_int_distribution<> dist(min_ins, max_ins);
        int instructions = dist(gen);
        
        add_process(name, instructions);
    }
}

std::shared_ptr<Process> ProcessManager::get_process(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = process_map.find(name);
    return it != process_map.end() ? it->second : nullptr;
}

bool ProcessManager::all_processes_completed() const {
    std::lock_guard<std::mutex> lock(mutex);
    return std::all_of(processes.begin(), processes.end(),
        [](const auto& p) { return p->is_finished(); });
}

void ProcessManager::print_system_status() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "%m/%d/%Y %I:%M:%S%p", std::localtime(&now_time));
    
    std::cout << "\nSystem Status - " << time_str << "\n";
    std::cout << "==================================\n";
    
    // CPU Utilization
    double total_util = 0.0;
    int busy_cores = 0;
    for (int i = 0; i < total_cores; ++i) {
        double util = cpu_util->get_utilization(i);
        total_util += util;
        if (util > 0.0) busy_cores++;
    }
    
    std::cout << "CPU utilization: " << std::fixed << std::setprecision(2) 
              << (total_util / total_cores) << "%\n";
    std::cout << "Cores used: " << busy_cores << "\n";
    std::cout << "Cores available: " << (total_cores - busy_cores) << "\n\n";
    
    // Running processes
    std::cout << "Running processes:\n";
    for (const auto& proc : processes) {
        if (!proc->is_finished()) {
            proc->print_short_status();
        }
    }
    
    // Finished processes
    std::cout << "\nFinished processes:\n";
    for (const auto& proc : processes) {
        if (proc->is_finished()) {
            proc->print_short_status();
        }
    }
    std::cout << std::endl;
}

void ProcessManager::generate_utilization_report() const {
    cpu_util->print_report();
}