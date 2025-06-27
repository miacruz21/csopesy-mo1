#pragma once
#include <deque>
#include <memory>
#include <mutex>
#include "process.h"

class SchedulerBase {
public:
    virtual void add_process(std::shared_ptr<Process> p) = 0;
    virtual std::shared_ptr<Process> next_process() = 0;
    virtual bool has_processes() const = 0;
    virtual void reset() = 0;
    virtual ~SchedulerBase() = default;
};

class FCFSScheduler : public SchedulerBase {
    mutable std::mutex mtx;
    std::deque<std::shared_ptr<Process>> q;
public:
    void add_process(std::shared_ptr<Process> p) override;
    std::shared_ptr<Process> next_process() override;
    bool has_processes() const override;
    void reset() override;
};

class RRScheduler : public SchedulerBase {
    mutable std::mutex mtx;
    std::deque<std::shared_ptr<Process>> q;
    uint64_t quantum;
public:
    explicit RRScheduler(uint64_t q);
    void add_process(std::shared_ptr<Process> p) override;
    std::shared_ptr<Process> next_process() override;
    bool has_processes() const override;
    void reset() override;
};