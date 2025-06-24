#include "core/instruction.hpp"
#include "core/process.hpp"
#include <stdexcept>
#include <thread>
#include <chrono>

PrintInstruction::PrintInstruction(const std::string& msg) : message(msg) {}

void PrintInstruction::execute(Process& process) {
    process.print(message);
}

std::string PrintInstruction::to_string() const {
    return "PRINT(" + message + ")";
}

DeclareInstruction::DeclareInstruction(const std::string& var, uint16_t value) 
    : var(var), value(value) {}

void DeclareInstruction::execute(Process& process) {
    process.set_variable(var, value);
}

std::string DeclareInstruction::to_string() const {
    return "DECLARE(" + var + ", " + std::to_string(value) + ")";
}

AddInstruction::AddInstruction(const std::string& var1, const std::string& var2, const std::string& var3)
    : var1(var1), var2(var2), var3(var3) {}

void AddInstruction::execute(Process& process) {
    uint16_t val1 = process.get_variable(var1);
    uint16_t val2 = process.get_variable(var2);
    process.set_variable(var3, val1 + val2);
}

std::string AddInstruction::to_string() const {
    return "ADD(" + var1 + ", " + var2 + ", " + var3 + ")";
}

SubtractInstruction::SubtractInstruction(const std::string& var1, const std::string& var2, const std::string& var3)
    : var1(var1), var2(var2), var3(var3) {}

void SubtractInstruction::execute(Process& process) {
    uint16_t val2 = process.get_variable_or_value(var2);
    uint16_t val3 = process.get_variable_or_value(var3);
    int32_t result = static_cast<int32_t>(val2) - static_cast<int32_t>(val3);
    process.set_variable(var1, process.clamp_value(result));
}

std::string SubtractInstruction::to_string() const {
    return "SUB(" + var1 + ", " + var2 + ", " + var3 + ")";
}

SleepInstruction::SleepInstruction(uint8_t ticks) : ticks(ticks) {}

void SleepInstruction::execute(Process& process) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ticks * 100));
}

std::string SleepInstruction::to_string() const {
    return "SLEEP(" + std::to_string(ticks) + ")";
}

ForLoopInstruction::ForLoopInstruction(std::vector<std::unique_ptr<Instruction>> instructions, uint16_t repeats)
    : instructions(std::move(instructions)), repeats(repeats) {}

void ForLoopInstruction::execute(Process& process) {
    for (uint16_t i = 0; i < repeats; ++i) {
        for (auto& instr : instructions) {
            instr->execute(process);
        }
    }
}

std::string ForLoopInstruction::to_string() const {
    return "FOR(instructions=" + std::to_string(instructions.size()) + 
           ", repeats=" + std::to_string(repeats) + ")";
}