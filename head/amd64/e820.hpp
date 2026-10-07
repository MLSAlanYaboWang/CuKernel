#if !defined(E820_HPP)
#define E820_HPP

#include "types.hpp"

struct E820Entry {
    uint64 base;
    uint64 length;
    uint32 type;
    uint32 acpi;
} __attribute__((packed));

inline uint32 e820GetCount() {
    return *(uint32*)0x5000;
}

inline E820Entry* e820GetEntries() {
    return (E820Entry*)0x5004;
}

inline E820Entry* e820GetEntry(uint32 i) {
    return (E820Entry*)(uint64)(0x5004 + i * 24);
}

#endif