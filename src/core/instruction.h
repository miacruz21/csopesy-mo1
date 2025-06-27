#pragma once
#include <string>
#include <vector>
#include <memory>

class Process;

struct Instruction {
    virtual ~Instruction() = default;
    virtual void execute(Process& p) = 0;
    virtual std::string to_string() const = 0;
};

struct PrintInst : Instruction {
    std::string msg;
    PrintInst(std::string m) : msg(std::move(m)) {}
    void execute(Process& p) override;
    std::string to_string() const override { return "PRINT(" + msg + ")"; }
};

struct DeclareInst : Instruction {
    std::string var;
    int val;
    DeclareInst(std::string v, int x) : var(std::move(v)), val(x) {}
    void execute(Process& p) override;
    std::string to_string() const override { return "DECLARE(" + var + "," + std::to_string(val) + ")"; }
};

struct MathInst : Instruction {
    bool is_add;
    std::string dest, op1, op2;
    MathInst(bool a, std::string d, std::string o1, std::string o2)
      : is_add(a), dest(std::move(d)), op1(std::move(o1)), op2(std::move(o2)) {}
    void execute(Process& p) override;
    std::string to_string() const override {
        return (is_add ? "ADD" : "SUB") + std::string("(") + dest + "," + op1 + "," + op2 + ")";
    }
};

struct SleepInst : Instruction {
    int ticks;
    explicit SleepInst(int t): ticks(t) {}
    void execute(Process& p) override;
    std::string to_string() const override { return "SLEEP(" + std::to_string(ticks) + ")"; }
};

struct ForInst : Instruction {
    std::vector<std::unique_ptr<Instruction>> body;
    int repeats;
    mutable int current = 0, index = 0;
    ForInst(std::vector<std::unique_ptr<Instruction>> b, int r)
      : body(std::move(b)), repeats(r) {}
    void execute(Process& p) override;
    std::string to_string() const override { return "FOR(...," + std::to_string(repeats) + ")"; }
};