#iif !defined(VBE_CONSOLE_HPP)
#define VBE_CONSOLE_HPP

#include "types.hpp"
#include "vmm.hpp"

namespace vbe {

struct Info {
    uint32 physBase;
    uint16 width;
    uint16 height;
    uint8  bpp;
    uint16 pitch;
};

inline Info info;
inline volatile uint8* fb = nullptr;
inline bool enabled = false;

inline void init() {
    info.physBase = *(uint32*)(uintptr)0x6000;
    info.width    = *(uint16*)(uintptr)0x6004;
    info.height   = *(uint16*)(uintptr)0x6006;
    info.bpp      = *(uint8*)(uintptr)0x6008;
    info.pitch    = *(uint16*)(uintptr)0x600A;

    if (info.width == 0 || info.height == 0) {
        enabled = false;
        return;
    }

    // 映射显存
    uint64 page = info.physBase & ~0xFFFULL;
    uint64 size = (uint64)info.pitch * info.height;
    uint64 pages = (size + 0xFFF) / 0x1000;

    for (uint64 i = 0; i < pages; ++i) {
        mapPage(page + i * 0x1000, page + i * 0x1000, PTE_PRESENT | PTE_WRITABLE);
    }

    fb = (volatile uint8*)(uintptr)info.physBase;
    enabled = true;
}

inline void putPixel(uint32 x, uint32 y, uint32 color) {
    if (!enabled) return;
    if (x >= info.width || y >= info.height) return;
    uint32 offset = y * info.pitch + x * (info.bpp / 8);
    fb[offset]     = color & 0xFF;
    fb[offset + 1] = (color >> 8) & 0xFF;
    fb[offset + 2] = (color >> 16) & 0xFF;
}

inline void fillRect(uint32 x, uint32 y, uint32 w, uint32 h, uint32 color) {
    if (!enabled) return;
    for (uint32 yy = 0; yy < h; ++yy) {
        for (uint32 xx = 0; xx < w; ++xx) {
            putPixel(x + xx, y + yy, color);
        }
    }
}

inline void clear(uint32 color) {
    if (!enabled) return;
    fillRect(0, 0, info.width, info.height, color);
}

}

#endif