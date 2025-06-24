#pragma once
#include "process.hpp"
#include <memory>
#include <vector>

class Scheduler {
public:
    virtual ~Scheduler() = default;
    virtual void add_process(std::shared_ptr<Process> process) = 0;
    virtual std::shared_ptr<Process> next_process() = 0;
    virtual bool has_processes() const = 0;
    virtual void reset() = 0;
};

class FCFSScheduler : public Scheduler {
    std::vector<std::shared_ptr<Process>> queue;
    mutable std::mutex mutex;
public:
    void add_process(std::shared_ptr<Process> process) override;
    std::shared_ptr<Process> next_process() override;
    bool has_processes() const override;
    void reset() override;
};

class RRScheduler : public Scheduler {
    std::vector<std::shared_ptr<Process>> queue;
    size_t current_index = 0;
    int quantum;
    mutable std::mutex mutex;
public:
    explicit RRScheduler(int quantum);
    void add_process(std::shared_ptr<Process> process) override;
    std::shared_ptr<Process> next_process() override;
    bool has_processes() const override;
    void reset() override;
};