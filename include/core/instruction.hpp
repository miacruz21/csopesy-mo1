#pragma once
#include <string>
#include <memory>
#include <vector>
#include "process.hpp"

class Process;

class Instruction {
public:
    virtual ~Instruction() = default;
    virtual void execute(Process& process) = 0;
    virtual std::string to_string() const = 0;
};

class PrintInstruction : public Instruction {
    std::string message;
public:
    explicit PrintInstruction(const std::string& msg);
    void execute(Process& process) override;
    std::string to_string() const override;
};

class DeclareInstruction : public Instruction {
    std::string var;
    uint16_t value;
public:
    DeclareInstruction(const std::string& var, uint16_t value);
    void execute(Process& process) override;
    std::string to_string() const override;
};

class AddInstruction : public Instruction {
    std::string var1, var2, var3;
public:
    AddInstruction(const std::string& var1, const std::string& var2, const std::string& var3);
    void execute(Process& process) override;
    std::string to_string() const override;
};

class SubtractInstruction : public Instruction {
    std::string var1, var2, var3;
public:
    SubtractInstruction(const std::string& var1, const std::string& var2, const std::string& var3);
    void execute(Process& process) override;
    std::string to_string() const override;
};

class SleepInstruction : public Instruction {
    uint8_t ticks;
public:
    explicit SleepInstruction(uint8_t ticks);
    void execute(Process& process) override;
    std::string to_string() const override;
};

class ForLoopInstruction : public Instruction {
    std::vector<std::unique_ptr<Instruction>> instructions;
    uint16_t repeats;
public:
    ForLoopInstruction(std::vector<std::unique_ptr<Instruction>> instructions, uint16_t repeats);
    void execute(Process& process) override;
    std::string to_string() const override;
};