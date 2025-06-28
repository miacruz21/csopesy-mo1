#pragma once
#include <string>
#include <vector>
#include <memory>

class Process;            

class Instruction {
public:
    virtual ~Instruction() = default;

    // run one step on a Process
    virtual void execute(Process&) = 0;

    // short opcode name for logging  (NEW)
    virtual const char* tag() const = 0;
};


class PrintInst : public Instruction {
    std::string msg;
public:
    explicit PrintInst(std::string m) : msg(std::move(m)) {}
    void        execute(Process& p) override;
    const char* tag() const override;             // "PRINT"
    const std::string& get_msg() const { return msg; }
};

class DeclInst : public Instruction {
    std::string var; int value;
public:
    DeclInst(std::string v, int val) : var(std::move(v)), value(val) {}
    void        execute(Process& p) override;
    const char* tag() const override;             // "DECL"
};

class MathInst : public Instruction {
    std::string dest, op1, op2;
    bool is_add;                                  // true=ADD, false=SUB
public:
    MathInst(std::string d,std::string a,std::string b,bool add)
        : dest(std::move(d)), op1(std::move(a)), op2(std::move(b)), is_add(add) {}
    void        execute(Process& p) override;
    const char* tag() const override;             // "ADD"/"SUB"
};

class SleepInst : public Instruction {
    int ticks;
public:
    explicit SleepInst(int t) : ticks(t) {}
    void        execute(Process& p) override;
    const char* tag() const override;             // "SLEEP"
};

class ForInst : public Instruction {
    int repeats, current = 0, index = 0;
    std::vector<std::unique_ptr<Instruction>> body;
public:
    ForInst(int r, std::vector<std::unique_ptr<Instruction>> b)
        : repeats(r), body(std::move(b)) {}
    void        execute(Process& p) override;
    const char* tag() const override;             // "FOR"
};
