#if !defined(PCI_HPP)
#define PCI_HPP

#include "types.hpp"
#include "io.hpp"

inline uint32 pciConfigRead(uint8 bus, uint8 slot, uint8 func, uint8 offset) {
    uint32 addr = 0x80000000
                | ((uint32)bus  << 16)
                | ((uint32)slot << 11)
                | ((uint32)func << 8)
                | (offset & 0xFC);
    outl(0xCF8, addr);
    return inl(0xCFC);
}

inline void pciConfigWrite(uint8 bus, uint8 slot, uint8 func, uint8 offset, uint32 val) {
    uint32 addr = 0x80000000
                | ((uint32)bus  << 16)
                | ((uint32)slot << 11)
                | ((uint32)func << 8)
                | (offset & 0xFC);
    outl(0xCF8, addr);
    outl(0xCFC, val);
}

struct PciDevice {
    uint8  bus, slot, func;
    uint16 vendorId, deviceId;
    uint8  classCode, subclass;
    uint8  progIf;
};

inline bool pciFindAhci(PciDevice* out) {
    *out = {};
    for (uint16 bus = 0; bus < 256; ++bus) {
        for (uint8 slot = 0; slot < 32; ++slot) {
            for (uint8 func = 0; func < 8; ++func) {
                uint32 id = pciConfigRead(bus, slot, func, 0x00);
                uint16 vendor = id & 0xFFFF;
                if (vendor == 0xFFFF) continue;

                uint32 classReg = pciConfigRead(bus, slot, func, 0x08);
                uint8 cls     = (classReg >> 24) & 0xFF;
                uint8 subcls  = (classReg >> 16) & 0xFF;
                uint8 progIf  = (classReg >> 8)  & 0xFF;

                if (cls == 0x01 && subcls == 0x06 && progIf == 0x01) {
                    out->bus = bus;
                    out->slot = slot;
                    out->func = func;
                    out->vendorId = vendor;
                    out->deviceId = (id >> 16) & 0xFFFF;
                    out->classCode = cls;
                    out->subclass = subcls;
                    out->progIf = progIf;
                    return true;
                }
            }
        }
    }
    return false;
}

inline uint32 pciReadBar(PciDevice* dev, uint8 bar) {
    return pciConfigRead(dev->bus, dev->slot, dev->func, 0x10 + bar * 4);
}

#endif