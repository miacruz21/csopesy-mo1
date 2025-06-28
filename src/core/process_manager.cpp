#include "process_manager.h"
#include "process.h"
#include "time_utils.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <bits/basic_string.h>

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
    const auto cores = util.get_total_cores();

    for (uint32_t core = 0; core < cores; ++core) {
        workers_.emplace_back([this, core]() {
            while (running) {
                std::shared_ptr<Process> p;
                {                               
                    std::lock_guard<std::mutex> lk(procs_mutex);
                    if (sched && sched->has_processes())
                        p = sched->next_process();
                }

                if (!p) {
                    util.mark_idle(core);                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                util.mark_busy(core);                 
                p->set_core_id(core);

                const uint64_t q = scheduler_is_rr_ ? rr_quantum_cycles_ : 1;
                for (uint64_t i = 0; i < q && !p->is_finished(); ++i) {
                    p->run_one_tick();
                    std::this_thread::sleep_for(std::chrono::milliseconds(30));
                }

                p->set_core_id(-1);
                util.mark_idle(core);                       

                if (!p->is_finished()) {
                    std::lock_guard<std::mutex> lk(procs_mutex);
                    sched->add_process(p);
                }
            }
        });
    }
}


void ProcessManager::stop_scheduler()
{
    running = false;
    for (auto& t : workers_)
        if (t.joinable()) t.join();
    workers_.clear();
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

void ProcessManager::print_system_status(std::ostream& out) const
{
    const auto total = util.get_total_cores();
    const auto busy  = util.get_busy_cores();

    out << "CPU utilization : "
        << std::fixed << std::setprecision(1)
        << util.get_utilization_percent() << " %\n"
        << "Cores used      : " << busy  << '/' << total << '\n'
        << "Cores available : " << (total - busy) << "\n\n";
}

void ProcessManager::print_recent_logs(std::ostream& out,
                                       std::size_t max_lines) const
{
    std::vector<std::string> all;
    {
        std::lock_guard<std::mutex> lk(procs_mutex);
        for (const auto& p : procs) {
            const auto& v = p->recent_logs(max_lines);   
            all.insert(all.end(), v.begin(), v.end());   
        }
    }  

    if (all.size() > max_lines)
        all.erase(all.begin(), all.end() - max_lines);

    out << "\nRecent logs (newest first, max " << max_lines << "):\n";
    for (auto it = all.rbegin(); it != all.rend(); ++it)
        out << "  " << *it << '\n';
}

void ProcessManager::print_process_lists(std::ostream& out,
                                         bool full) const
{

    std::vector<std::shared_ptr<Process>> running, finished;
    {
        std::lock_guard<std::mutex> lk(procs_mutex);
        running.reserve(procs.size());
        finished.reserve(procs.size());
        for (auto const& p : procs)
            (p->is_finished() ? finished : running).push_back(p);
    } 

    auto dump = [&](auto const& vec, std::string_view title)
    {
        out << title << '\n';
        std::size_t shown = 0;
        const bool isRunning = title.compare(0, 7, "Running") == 0;

        for (auto const& p : vec) {
            if (!full && shown == 5) { out << "…\n"; break; }

            if (isRunning) {
                out << std::left << std::setw(15) << p->get_name() << ' '
                    << util::now_time() << "  Core:" << p->get_core_id() << "  "
                    << p->get_pc() << '/' << p->get_code_size() << '\n';
            } else {
                out << std::left << std::setw(15) << p->get_name() << ' '
                    << p->get_finished_time() << "  FINISHED  "
                    << p->get_code_size() << '/' << p->get_code_size() << '\n';
            }
            ++shown;
        }
        out << '\n';
    };


    dump(running , "Running processes:");
    dump(finished, "Finished processes:");
    out << "___________________________________________________________\n";
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

    for (auto& t : workers_)
        if (t.joinable()) t.join();
    workers_.clear();

    if (batch_thread.joinable())
        batch_thread.join();
}
