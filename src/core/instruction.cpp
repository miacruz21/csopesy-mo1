#include "instruction.h"
#include "process.h"
#include "time_utils.h"
#include <sstream>
#include <cctype>

extern std::string now_time();

void PrintInst::execute(Process& p)
{
    std::ostringstream oss;
    oss << "(" << util::now_time() << ") Core:"  
        << p.get_core_id() << " \"" << msg << "\"";
}

void DeclInst::execute(Process& p) { 
    p.set_var(var, value);
}

void MathInst::execute(Process& p) {
    int a = p.get_var_or_val(op1);
    int b = p.get_var_or_val(op2);
    long long res64 = is_add ? (static_cast<long long>(a) + b)
                             : (static_cast<long long>(a) - b);
    if (res64 < 0)     res64 = 0;
    if (res64 > 65535) res64 = 65535;
    p.set_var(dest, static_cast<int>(res64));
}

void SleepInst::execute(Process& p) {
    p.sleep(ticks);
}

void ForInst::execute(Process& p) {
    if (repeats <= 0) return; 
    if (current < repeats) {
        if (index < body.size()) {
            body[index++]->execute(p);
        } else {
            ++current; index = 0;
        }
    }
}

const char* PrintInst::tag() const  { return "PRINT"; }
const char* DeclInst::tag()  const  { return "DECL";  }
const char* MathInst::tag()  const  { return is_add ? "ADD" : "SUB"; }
const char* SleepInst::tag() const  { return "SLEEP"; }
const char* ForInst::tag()   const  { return "FOR";   }