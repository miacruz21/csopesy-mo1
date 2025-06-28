#include "process_manager.h"
#include "process.h"
#include "time_utils.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <filesystem>

extern std::atomic<uint64_t> cpu_cycles_counter;


static std::string format_percent(double v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << v;
    return oss.str();
}

ProcessManager::ProcessManager(uint32_t cores)
    : util(cores) {}

ProcessManager::~ProcessManager()
{
    stop_batch_processing();
    stop_scheduler();
}

void ProcessManager::set_config_manager(ConfigManager *c) { cfg = c; }

void ProcessManager::initialize_scheduler(const std::string &alg, uint64_t quantum)
{
    if (alg == "fcfs") {
        sched = std::make_unique<FCFSScheduler>();
        scheduler_is_rr_ = false;
    } else {
        sched = std::make_unique<RRScheduler>(quantum);
        scheduler_is_rr_   = true;
        rr_quantum_cycles_ = quantum;
    }
}

void ProcessManager::start_scheduler()
{
    running = true;
    sched_thread = std::thread([this]()
                               {
        while (running) {
            cpu_cycles_counter++;
            std::shared_ptr<Process> p;
            uint64_t quantum = scheduler_is_rr_ ? rr_quantum_cycles_ : 1;
            {
                std::lock_guard<std::mutex> lk(procs_mutex);
                if (sched != nullptr && sched->has_processes())
                    p = sched->next_process();
            }
            if (p) {
                util.mark_busy(0);
                for (uint64_t i = 0; i < quantum && !p->is_finished(); ++i) {
                    p->run_one_tick();
                    std::this_thread::sleep_for(std::chrono::milliseconds(30));
                }
                util.mark_idle(0);  
                if (!p->is_finished()) {
                    std::lock_guard<std::mutex> lk(procs_mutex);
                    sched->add_process(p);
                }
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }

        } });
}

void ProcessManager::stop_scheduler()
{
    running = false;
    if (sched_thread.joinable())
        sched_thread.join();
}

void ProcessManager::start_batch_processing()
{
    if (!cfg)
        return;
    batching = true;
    uint64_t freq = cfg->get_long("batch-process-freq");
    uint64_t delay = cfg->get_long("delays-per-exec");
    uint64_t min_ins = cfg->get_long("min-ins");
    uint64_t max_ins = cfg->get_long("max-ins");

    batch_thread = std::thread([this, freq, delay, min_ins, max_ins]()
                               {
        while (batching) {
            if ((cpu_cycles_counter % freq) == 0) {
                std::string name = "p" + std::to_string(next_id);
                auto p = std::make_shared<Process>(name, next_id++, min_ins, max_ins, delay);
                {
                    std::lock_guard<std::mutex> lk(procs_mutex);
                    procs.push_back(p);
                    if (sched) sched->add_process(p);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        } });
}

void ProcessManager::stop_batch_processing()
{
    batching = false;
    if (batch_thread.joinable())
        batch_thread.join();
}

std::shared_ptr<Process> ProcessManager::get_process(const std::string &name) const
{
    std::lock_guard<std::mutex> lk(procs_mutex);
    for (auto &p : procs)
        if (p->get_name() == name)
            return p;
    return nullptr;
}

void ProcessManager::add_process(const std::string &name)
{
    get_or_create_process(name);
}

std::shared_ptr<Process> ProcessManager::get_or_create_process(const std::string &name)
{
    auto p = get_process(name);
    if (p)
        return p;
    if (!cfg)
        return nullptr;
    uint32_t delay = cfg->get_long("delays-per-exec");
    uint32_t min_ins = cfg->get_long("min-ins");
    uint32_t max_ins = cfg->get_long("max-ins");
    p = std::make_shared<Process>(name, next_id++, min_ins, max_ins, delay);
    {
        std::lock_guard<std::mutex> lk(procs_mutex);
        procs.push_back(p);
        if (sched)
            sched->add_process(p);
    }
    return p;
}

void ProcessManager::print_system_status(std::ostream& out) const {
    size_t running = 0, finished = 0;
    {
        std::lock_guard<std::mutex> lk(procs_mutex);
        for (const auto& proc : procs)
            if (proc->is_finished()) ++finished; else ++running;
    }

    // Get CPU utilization and core stats
    double util_percent = util.get_utilization_percent();
    int cores_used = util.get_busy_cores();
    int total = cores_used + util.get_available_cores();

    out << "\n--------------------------------------------------\n";
    out << "CPU utilization: " << format_percent(util_percent) << "%\n";
    out << "Cores used: " << cores_used << "\n";
    out << "Cores available: " << (total - cores_used) << "\n";
    out << "--------------------------------------------------\n";

    out << "Running processes:\n";
    out << std::left << std::setw(16) << "Process"
        << std::setw(24) << "Created"
        << std::setw(8) << "Core"
        << std::setw(16) << "Instr/Total"
        << "\n";

    {
        std::lock_guard<std::mutex> lk(procs_mutex);
        for (const auto& proc : procs) {
            if (!proc->is_finished()) {
                out << std::left << std::setw(16) << proc->get_name()
                    << std::setw(24) << proc->get_created_time()
                    << std::setw(8) << proc->get_core_id()
                    << std::setw(16) << (std::to_string(proc->get_pc()) + "/" + std::to_string(proc->get_code_size()))
                    << "\n";
            }
        }
    }

    out << "\nFinished processes:\n";
    out << std::left << std::setw(16) << "Process"
        << std::setw(24) << "Finished"
        << std::setw(16) << "Instr/Total"
        << "\n";
    {
        std::lock_guard<std::mutex> lk(procs_mutex);
        for (const auto& proc : procs) {
            if (proc->is_finished()) {
                out << std::left << std::setw(16) << proc->get_name()
                    << std::setw(24) << proc->get_finished_time()
                    << std::setw(16) << (std::to_string(proc->get_code_size()) + "/" + std::to_string(proc->get_code_size()))
                    << "\n";
            }
        }
    }
    out << "--------------------------------------------------\n";
}

void ProcessManager::print_system_status() const {
    print_system_status(std::cout);
}

void ProcessManager::generate_utilization_report() const
{
    std::ofstream ofs("csopesy-log.txt", std::ios::app);
    ofs << "CPU utilization: " << std::fixed << std::setprecision(1)
    << util.get_utilization_percent() << "%\n"
    << "Cores used: "      << util.get_busy_cores()      << '\n'
    << "Cores available: " << util.get_available_cores() << "\n\n";
}

void ProcessManager::shutdown()
{
    running = false;                  
    if (sched_thread.joinable())
        sched_thread.join();

    if (batch_thread.joinable())
        batch_thread.join();
}