#if !defined(REGS_HPP)
#define REGS_HPP

#include "types.hpp"

struct Regs {
    uint64 r15, r14, r13, r12, r11, r10, r9, r8;
    uint64 rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64 intNo, errorCode;
    uint64 rip, cs, rflags, rsp, ss;
};

#endif