#if !defined(IDT_HPP)
#define IDT_HPP

#include "types.hpp"
#include "io.hpp"

struct IDTEntry {
    uint16 baseLow;
    uint16 sel;
    uint8  ist;
    uint8  flags;
    uint16 baseMid;
    uint32 baseHigh;
    uint32 reserved;
} __attribute__((packed));

struct IDTPtr {
    uint16 limit;
    uint64 base;
} __attribute__((packed));

extern "C" {
    void isr0();  void isr1();  void isr2();  void isr3();
    void isr4();  void isr5();  void isr6();  void isr7();
    void isr8();  void isr9();  void isr10(); void isr11();
    void isr12(); void isr13(); void isr14(); void isr15();
    void isr16(); void isr17(); void isr18(); void isr19();
    void isr20(); void isr21(); void isr22(); void isr23();
    void isr24(); void isr25(); void isr26(); void isr27();
    void isr28(); void isr29(); void isr30(); void isr31();
    void isr32(); void isr33(); void isr34(); void isr35();
    void isr36(); void isr37(); void isr38(); void isr39();
    void isr40(); void isr41(); void isr42(); void isr43();
    void isr44(); void isr45(); void isr46(); void isr47();
}

inline IDTEntry idtTable[256];
inline IDTPtr   idtPtr;

inline void idtSetGate(uint8 num, uint64 base, uint16 sel, uint8 flags) {
    idtTable[num].baseLow  = base & 0xFFFF;
    idtTable[num].baseMid  = (base >> 16) & 0xFFFF;
    idtTable[num].baseHigh = (base >> 32) & 0xFFFFFFFF;
    idtTable[num].sel      = sel;
    idtTable[num].ist      = 0;
    idtTable[num].flags    = flags;
    idtTable[num].reserved = 0;
}

inline void picRemap() {
    outb(0x20, 0x11); ioWait();
    outb(0xA0, 0x11); ioWait();
    outb(0x21, 0x20); ioWait();
    outb(0xA1, 0x28); ioWait();
    outb(0x21, 0x04); ioWait();
    outb(0xA1, 0x02); ioWait();
    outb(0x21, 0x01); ioWait();
    outb(0xA1, 0x01); ioWait();
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

inline void idtInit() {
    idtPtr.limit = sizeof(IDTEntry) * 256 - 1;
    idtPtr.base  = (uint64)&idtTable;

    for (int i = 0; i < 256; ++i)
        idtSetGate(i, 0, 0, 0);

    picRemap();

    uint8  flags = 0x8E;    // P=1, DPL=0, 32位中断门
    uint16 sel   = 0x08;    // 64 位代码段选择子

    idtSetGate( 0, (uint64)isr0,  sel, flags);
    idtSetGate( 1, (uint64)isr1,  sel, flags);
    idtSetGate( 2, (uint64)isr2,  sel, flags);
    idtSetGate( 3, (uint64)isr3,  sel, flags);
    idtSetGate( 4, (uint64)isr4,  sel, flags);
    idtSetGate( 5, (uint64)isr5,  sel, flags);
    idtSetGate( 6, (uint64)isr6,  sel, flags);
    idtSetGate( 7, (uint64)isr7,  sel, flags);
    idtSetGate( 8, (uint64)isr8,  sel, flags);
    idtSetGate( 9, (uint64)isr9,  sel, flags);
    idtSetGate(10, (uint64)isr10, sel, flags);
    idtSetGate(11, (uint64)isr11, sel, flags);
    idtSetGate(12, (uint64)isr12, sel, flags);
    idtSetGate(13, (uint64)isr13, sel, flags);
    idtSetGate(14, (uint64)isr14, sel, flags);
    idtSetGate(15, (uint64)isr15, sel, flags);
    idtSetGate(16, (uint64)isr16, sel, flags);
    idtSetGate(17, (uint64)isr17, sel, flags);
    idtSetGate(18, (uint64)isr18, sel, flags);
    idtSetGate(19, (uint64)isr19, sel, flags);
    idtSetGate(20, (uint64)isr20, sel, flags);
    idtSetGate(21, (uint64)isr21, sel, flags);
    idtSetGate(22, (uint64)isr22, sel, flags);
    idtSetGate(23, (uint64)isr23, sel, flags);
    idtSetGate(24, (uint64)isr24, sel, flags);
    idtSetGate(25, (uint64)isr25, sel, flags);
    idtSetGate(26, (uint64)isr26, sel, flags);
    idtSetGate(27, (uint64)isr27, sel, flags);
    idtSetGate(28, (uint64)isr28, sel, flags);
    idtSetGate(29, (uint64)isr29, sel, flags);
    idtSetGate(30, (uint64)isr30, sel, flags);
    idtSetGate(31, (uint64)isr31, sel, flags);
    idtSetGate(32, (uint64)isr32, sel, flags);
    idtSetGate(33, (uint64)isr33, sel, flags);
    idtSetGate(34, (uint64)isr34, sel, flags);
    idtSetGate(35, (uint64)isr35, sel, flags);
    idtSetGate(36, (uint64)isr36, sel, flags);
    idtSetGate(37, (uint64)isr37, sel, flags);
    idtSetGate(38, (uint64)isr38, sel, flags);
    idtSetGate(39, (uint64)isr39, sel, flags);
    idtSetGate(40, (uint64)isr40, sel, flags);
    idtSetGate(41, (uint64)isr41, sel, flags);
    idtSetGate(42, (uint64)isr42, sel, flags);
    idtSetGate(43, (uint64)isr43, sel, flags);
    idtSetGate(44, (uint64)isr44, sel, flags);
    idtSetGate(45, (uint64)isr45, sel, flags);
    idtSetGate(46, (uint64)isr46, sel, flags);
    idtSetGate(47, (uint64)isr47, sel, flags);

    asm volatile("lidt %0" : : "m"(idtPtr));
}

#endif