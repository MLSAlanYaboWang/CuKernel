#if !defined(IO_HPP)
#define IO_HPP

#include "types.hpp"

static inline void outb(uint16 port, uint8 val) {
    asm volatile("outb %0, %1" : : "a"(val), "d"(port));
}

static inline uint8 inb(uint16 port) {
    uint8 ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "d"(port));
    return ret;
}

static inline uint16 inw(uint16 port) {
    uint16 ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "d"(port));
    return ret;
}

static inline void outw(uint16 port, uint16 val) {
    asm volatile("outw %0, %1" : : "a"(val), "d"(port));
}

static inline uint32 inl(uint16 port) {
    uint32 ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "d"(port));
    return ret;
}

static inline void outl(uint16 port, uint32 val) {
    asm volatile("outl %0, %1" : : "a"(val), "d"(port));
}

static inline void ioWait() {
    outb(0x80, 0);
}

#endif