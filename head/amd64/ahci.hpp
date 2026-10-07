#if !defined(AHCI_HPP)
#define AHCI_HPP

#include "types.hpp"
#include "io.hpp"
#include "pci.hpp"
#include "vmm.hpp"
#include "pmm.hpp"

static const uint32 AHCI_CAP    = 0x00;   // Host Capabilities
static const uint32 AHCI_GHC    = 0x04;   // Global Host Control
static const uint32 AHCI_IS     = 0x08;   // Interrupt Status
static const uint32 AHCI_PI     = 0x0C;   // Ports Implemented
static const uint32 AHCI_VS     = 0x10;   // Version
static const uint32 AHCI_CCC_CTL= 0x14;
static const uint32 AHCI_CCC_PORTS = 0x18;
static const uint32 AHCI_EM_LOC = 0x1C;
static const uint32 AHCI_EM_CTL = 0x20;
static const uint32 AHCI_CAP2   = 0x24;
static const uint32 AHCI_BOHC   = 0x28;

static const uint32 GHC_AE = 1 << 31;   // AHCI Enable

static const uint32 PORT_BASE = 0x100;
static const uint32 PORT_SIZE = 0x80;

static const uint32 PxCLB   = 0x00;   // Command List Base
static const uint32 PxCLBU  = 0x04;
static const uint32 PxFB    = 0x08;   // FIS Base
static const uint32 PxFBU   = 0x0C;
static const uint32 PxIS    = 0x10;   // Interrupt Status
static const uint32 PxIE    = 0x14;   // Interrupt Enable
static const uint32 PxCMD   = 0x18;   // Command and Status
static const uint32 PxTFD   = 0x20;   // Task File Data
static const uint32 PxSIG   = 0x24;   // Signature
static const uint32 PxSSTS  = 0x28;   // SATA Status
static const uint32 PxSCTL  = 0x2C;
static const uint32 PxSERR  = 0x30;
static const uint32 PxSACT  = 0x34;
static const uint32 PxCI    = 0x38;   // Command Issue
static const uint32 PxSNTF  = 0x3C;
static const uint32 PxFBS   = 0x40;

static const uint32 PxCMD_ST   = 1 << 0;
static const uint32 PxCMD_FRE  = 1 << 4;
static const uint32 PxCMD_FR   = 1 << 14;
static const uint32 PxCMD_CR   = 1 << 15;

static const uint32 PxTFD_BSY = 1 << 7;
static const uint32 PxTFD_DRQ = 1 << 3;

static const uint32 SATA_SIG_ATA = 0x00000101;
static const uint32 SATA_SIG_ATAPI = 0xEB140101;

inline uint32 mmioRead(volatile uint8* base, uint32 off) {
    return *(volatile uint32*)(base + off);
}

inline void mmioWrite(volatile uint8* base, uint32 off, uint32 val) {
    *(volatile uint32*)(base + off) = val;
}

struct AhciPort {
    volatile uint8* abar;
    uint8 portNo;
    uint64 clbPhys;
    uint64 fbPhys;
    uint64 ctPhys;
};

inline bool ahciInitPort(AhciPort* p) {
    uint32 off = PORT_BASE + p->portNo * PORT_SIZE;

    uint32 cmd = mmioRead(p->abar, off + PxCMD);
    cmd &= ~PxCMD_ST;
    mmioWrite(p->abar, off + PxCMD, cmd);

    cmd &= ~PxCMD_FRE;
    mmioWrite(p->abar, off + PxCMD, cmd);

    uint32 timeout = 1000000;
    while ((mmioRead(p->abar, off + PxCMD) & (PxCMD_CR | PxCMD_FR)) && --timeout) {}
    if (!timeout) return false;

    p->clbPhys = allocPage();
    p->fbPhys  = allocPage();
    p->ctPhys  = allocPage();
    if (!p->clbPhys || !p->fbPhys || !p->ctPhys) return false;

    for (int i = 0; i < 4096; ++i) {
        ((uint8*)(uintptr)p->clbPhys)[i] = 0;
        ((uint8*)(uintptr)p->fbPhys)[i]  = 0;
        ((uint8*)(uintptr)p->ctPhys)[i]  = 0;
    }

    mmioWrite(p->abar, off + PxCLB,  (uint32)p->clbPhys);
    mmioWrite(p->abar, off + PxCLBU, (uint32)(p->clbPhys >> 32));
    mmioWrite(p->abar, off + PxFB,   (uint32)p->fbPhys);
    mmioWrite(p->abar, off + PxFBU,  (uint32)(p->fbPhys >> 32));

    mmioWrite(p->abar, off + PxSERR, 0xFFFFFFFF);

    cmd = mmioRead(p->abar, off + PxCMD);
    cmd |= PxCMD_FRE;
    mmioWrite(p->abar, off + PxCMD, cmd);

    cmd |= PxCMD_ST;
    mmioWrite(p->abar, off + PxCMD, cmd);

    return true;
}

inline bool ahciRead(AhciPort* p, uint64 lba, uint32 count, uint8* buf) {
    uint32 off = PORT_BASE + p->portNo * PORT_SIZE;

    uint32 timeout = 1000000;
    while ((mmioRead(p->abar, off + PxTFD) & (PxTFD_BSY | PxTFD_DRQ)) && --timeout) {}
    if (!timeout) { printString("  A: BSY timeout\n"); return false; }

    uint8* ct = (uint8*)(uintptr)p->ctPhys;
    for (int i = 0; i < 256; ++i) ct[i] = 0;

    ct[0]  = 0x27;
    ct[1]  = 0x80;
    ct[2]  = 0x25;
    ct[4]  = lba & 0xFF;
    ct[5]  = (lba >> 8) & 0xFF;
    ct[6]  = (lba >> 16) & 0xFF;
    ct[7]  = 0x40;
    ct[8]  = (lba >> 24) & 0xFF;
    ct[9]  = (lba >> 32) & 0xFF;
    ct[10] = (lba >> 40) & 0xFF;
    ct[12] = count & 0xFF;
    ct[13] = (count >> 8) & 0xFF;

    uint64 dataPhys = getPhys((uint64)(uintptr)buf);    // FUCK!
    if (dataPhys == 0) {
        printString("  buf not mapped!\n");
        return false;
    }

    uint8* prdt = ct + 128;
    *(uint32*)(prdt + 0)  = (uint32)dataPhys;
    *(uint32*)(prdt + 4)  = (uint32)(dataPhys >> 32);
    *(uint32*)(prdt + 8)  = 0;
    *(uint32*)(prdt + 12) = (count * 512 - 1) | 1;

    printString("  B: virt=");
    printHex<16>((uint64)(uintptr)buf);
    printString(" phys=");
    printHex<16>(dataPhys);
    printString("\n");

    uint8* cl = (uint8*)(uintptr)p->clbPhys;
    for (int i = 0; i < 32; ++i) cl[i] = 0;
    cl[0] = 5;
    cl[2] = 1;
    *(uint32*)(cl + 8)  = (uint32)p->ctPhys;
    *(uint32*)(cl + 12) = (uint32)(p->ctPhys >> 32);

    mmioWrite(p->abar, off + PxCI, 1);

    timeout = 10000000;
    while ((mmioRead(p->abar, off + PxCI) & 1) && --timeout) {}
    if (!timeout) { printString("  C: PxCI timeout\n"); return false; }

    uint32 tfd   = mmioRead(p->abar, off + PxTFD);
    uint32 is    = mmioRead(p->abar, off + PxIS);
    uint32 prdbc = *(uint32*)(cl + 4);

    printString("  D: tfd=");
    printHex<16>(tfd);
    printString(" is=");
    printHex<16>(is);
    printString(" prdbc=");
    printHex<16>(prdbc);
    printString("\n");

    if (tfd & 0x01) { printString("  E: TFD ERR\n"); return false; }
    return true;
}

inline uint64 ahciGetAbar(PciDevice* dev) {
    uint32 bar5 = pciReadBar(dev, 5);
    uint64 base = bar5 & 0xFFFFFFF0ULL;
    if ((bar5 & 0x06) == 0x04) {
        uint32 bar6 = pciReadBar(dev, 6);
        base |= (uint64)bar6 << 32;
    }
    return base;
}

#endif