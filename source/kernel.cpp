#define AMD64
#include "../head/include.hpp"

extern "C" void kernelMain(void) __attribute__((section(".text.kernelMain")));

extern "C" {
    void d_demo(void);
    void d_hello(void);
    int  d_add(int, int);
}

static uint32 tickCount = 0;

extern "C" {
    extern uint8 _bssStart[];
    extern uint8 _bssEnd[];
}

static void clearBSS(void) {
    for (uint8* p = _bssStart; p < _bssEnd; ++p) *p = 0;
}

[[maybe_unused]] static void printE820() {
    uint32 count = e820GetCount();
    printString("E820 entries: ");
    printHex<16>(count);
    printString("\n");

    for (uint32 i = 0; i < count; ++i) {
        E820Entry* e = e820GetEntry(i);
        printString("  base=");
        printHex<16>(e->base);
        printString(" len=");
        printHex<16>(e->length);
        printString(" type=");
        printHex<16>(e->type);
        printString("\n");
    }
}

static void setupFormalPageTables() {
    uint64 cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    printString("CR3 = ");
    printHex<16>(cr3);
    printString("\n");

    uint64 cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    printString("CR0 = ");
    printHex<16>(cr0);
    printString("\n");

    uint64 cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    printString("CR4 = ");
    printHex<16>(cr4);
    printString("\n");
}

extern "C" void irqHandler(Regs* r) {
    uint64 intNo = r->intNo;

    if (intNo >= 32 && intNo <= 47) {
        uint64 irq = intNo - 32;

        if (irq == 0) {
            ++tickCount;
        } else if (irq == 1) {
            uint8 scanCode = inb(0x60);
            char c = keyboardHandle(scanCode);
            if (c) keyboardBufferPut(c);
        }

        if (irq >= 8) outb(0xA0, 0x20);
        outb(0x20, 0x20);
        return;
    }

    printString("\nEXCEPTION ");
    printHex<16>(intNo);
    printString(" err=");
    printHex<16>(r->errorCode);
    printString(" rip=");
    printHex<16>(r->rip);
    printString("\n");

    if (intNo == 14) {
        uint64 cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        printString("CR2=");
        printHex<16>(cr2);
        printString("\n");
    }

    while (true) asm volatile("hlt");
}

void dumpBytes(const uint8* buf, int n) {
    for (int i = 0; i < n; ++i) {
        printHex<16>(buf[i]);
        printString(" ");
        if ((i & 15) == 15) printString("\n");
    }
    printString("\n");
}

uint8 kernel(void) {
    printString("CuKernel\n");
    return 0;
}

extern "C" void kernelMain(void) {
    clearBSS();
    clearScreen();
    printString("Has entered the kernel\n");

    // printE820();

    idtInit();
    outb(0x21, 0xFC);
    printString("IDT okay.\n");
    asm volatile("sti");

    printInt(0);
    printChar('\n');

    uint32 lastTick = 0;

    setupFormalPageTables();

    pmmInit();

    heapInit();

    PciDevice ahci{};
    if (!pciFindAhci(&ahci)) {
        printString("AHCI disk not found!");
    }

    uint64 abarPhys = ahciGetAbar(&ahci);
    uint64 abarPage = abarPhys & ~0xFFFULL;
    for (uint64 off = 0; off < 0x2000; off += 0x1000) {
        mapPage(abarPage + off, abarPage + off, PTE_PRESENT | PTE_WRITABLE);
    }
    volatile uint8* abar = (volatile uint8*)(uintptr)abarPhys;

    uint32 ghc = mmioRead(abar, AHCI_GHC);
    mmioWrite(abar, AHCI_GHC, ghc | GHC_AE);

    uint8 targetPort = 0;
    AhciPort ap;
    ap.abar = abar;
    ap.portNo = targetPort;

    if (!ahciInitPort(&ap)) {
        printString("ahciInitPort failed\n");
        while (true) asm volatile("hlt");
    }
    printString("Port init OK\n");

    uint8* buf = (uint8*)kmalloc(512);
    if (ahciRead(&ap, 0, 1, buf)) {
        printString("MBR sig: ");
        printHex<2>(buf[510]);
        printString(" ");
        printHex<2>(buf[511]);
        printString("\n");
    }
    kfree(buf);

    FAT32 fs;
    if (fat32Init(&fs, &ap)) {
        printString("FAT32 OK\n");
        printString("Root dir:\n");
        fat32ListDir(&fs, fs.rootCluster);
    } else {
        printString("FAT32 init failed\n");
    }

    printString("\n");

    d_demo();

    printString("Done. Halting.\n");
    while (true) {
        asm volatile("hlt");

        cchar c;
        while (keyboardBufferGet(&c)) {
            printChar(c);
        }
        
        if (tickCount - lastTick >= 18) {
            lastTick = tickCount;
            // printString("t\n");    It's okay. No problem here.
        }
    }
}