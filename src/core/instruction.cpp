#include "instruction.h"
#include "process.h"
#include <sstream>
#include <cctype>

extern std::string now_time();

void PrintInst::execute(Process& p) {
    std::ostringstream oss;
    oss << "(" << now_time() << ") Core:" << p.get_core_id() << " \"" << msg << "\"";
    p.log(oss.str());
}

void DeclareInst::execute(Process& p) { p.set_var(var, val); }

void MathInst::execute(Process& p) {
    int a = p.get_var_or_val(op1);
    int b = p.get_var_or_val(op2);
    int r = is_add ? (a + b) : (a - b);
    p.set_var(dest, r);
}

void SleepInst::execute(Process& p) { p.sleep(ticks); }

void ForInst::execute(Process& p) {
    if (current < repeats) {
        if (index < body.size()) {
            body[index++]->execute(p);
        } else {
            ++current; index = 0;
        }
    }
}