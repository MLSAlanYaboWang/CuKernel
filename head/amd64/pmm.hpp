#if !defined(PMM_HPP)
#define PMM_HPP

#include "types.hpp"
#include "e820.hpp"

static const uint32 PAGE_SIZE = 4096;
static const uint32 MAX_PAGES = 1024 * 1024;

struct PMM {
    uint64  bitmapAddr;
    uint32  totalPages;
    uint32  usedPages;
    uint64  memStart;
    uint64  memEnd;
};

inline PMM pmm;

inline void pmmInit() {
    uint32 count = e820GetCount();

    uint64 bestBase = 0, bestLen = 0;
    for (uint32 i = 0; i < count; ++i) {
        E820Entry* e = e820GetEntry(i);
        if (e->type == 1 && e->length > bestLen) {
            bestBase = e->base;
            bestLen  = e->length;
        }
    }

    pmm.memStart   = bestBase;
    pmm.memEnd     = bestBase + bestLen;
    pmm.totalPages = (uint32)(bestLen / PAGE_SIZE);
    pmm.usedPages  = 0;

    uint32 bitmapBytes = pmm.totalPages / 8;
    pmm.bitmapAddr = pmm.memStart;

    uint8* bm = (uint8*)(uintptr)pmm.bitmapAddr;
    for (uint32 i = 0; i < bitmapBytes; ++i) bm[i] = 0;

    uint32 bitmapPages = (bitmapBytes + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32 i = 0; i < bitmapPages; ++i) {
        pmm.usedPages++;
        uint32 byteIdx = i / 8;
        uint32 bitIdx  = i % 8;
        bm[byteIdx] |= (1 << bitIdx);
    }
}

inline void pmmSetUsed(uint32 page, bool used) {
    uint8* bm = (uint8*)(uintptr)pmm.bitmapAddr;
    uint32 byteIdx = page / 8;
    uint32 bitIdx  = page % 8;
    if (used) bm[byteIdx] |= (1 << bitIdx);
    else      bm[byteIdx] &= ~(1 << bitIdx);
}

inline bool pmmIsUsed(uint32 page) {
    uint8* bm = (uint8*)(uintptr)pmm.bitmapAddr;
    return (bm[page / 8] >> (page % 8)) & 1;
}

inline uint64 allocPage() {
    for (uint32 i = 0; i < pmm.totalPages; ++i) {
        if (!pmmIsUsed(i)) {
            pmmSetUsed(i, true);
            pmm.usedPages++;
            return pmm.memStart + (uint64)i * PAGE_SIZE;
        }
    }
    return 0;
}

inline void freePage(uint64 phys) {
    if (phys < pmm.memStart || phys >= pmm.memEnd) return;
    uint32 page = (uint32)((phys - pmm.memStart) / PAGE_SIZE);
    if (pmmIsUsed(page)) {
        pmmSetUsed(page, false);
        pmm.usedPages--;
    }
}

#endif