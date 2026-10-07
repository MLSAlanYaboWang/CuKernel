#if !defined(GRAPHICS_HPP)
#define GRAPHICS_HPP

#include "types.hpp"

struct VbeInfo {
    uint32 physBase;
    uint16 width;
    uint16 height;
    uint8  bpp;
    uint16 pitch;
};

inline VbeInfo gfx;

inline void gfxInit() {
    gfx.physBase = *(uint32*)0x6000;
    gfx.width    = *(uint16*)0x6004;
    gfx.height   = *(uint16*)0x6006;
    gfx.bpp      = *(uint8*)0x6008;
    gfx.pitch    = *(uint16*)0x600A;
}

inline void gfxPutPixel(uint32 x, uint32 y, uint32 color) {
    uint8* fb = (uint8*)(uintptr)gfx.physBase;
    uint32 offset = y * gfx.pitch + x * (gfx.bpp / 8);
    fb[offset]     = color & 0xFF;
    fb[offset + 1] = (color >> 8) & 0xFF;
    fb[offset + 2] = (color >> 16) & 0xFF;
}

#endif