#if !defined(VMM_HPP)
#define VMM_HPP

#include "types.hpp"
#include "pmm.hpp"
#include "io.hpp"

static const uint64 PTE_PRESENT  = 1ULL << 0;
static const uint64 PTE_WRITABLE = 1ULL << 1;
static const uint64 PTE_USER     = 1ULL << 2;
static const uint64 PTE_PWT      = 1ULL << 3;
static const uint64 PTE_PCD      = 1ULL << 4;
static const uint64 PTE_ACCESSED = 1ULL << 5;
static const uint64 PTE_DIRTY    = 1ULL << 6;
static const uint64 PTE_HUGE     = 1ULL << 7;
static const uint64 PTE_GLOBAL   = 1ULL << 8;
static const uint64 PTE_NX       = 1ULL << 63;

static const uint64 ADDR_MASK    = 0x000FFFFFFFFFF000ULL;
static const uint64 HUGE_MASK    = 0x000FFFFFFFE00000ULL;
static const uint64 ENTRY_MASK   = 0x1FF;

struct PageTable {
    uint64 entries[512];
};

inline uint64 vmmGetCr3() {
    uint64 cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3 & ADDR_MASK;
}

inline void vmmSetCr3(uint64 phys) {
    asm volatile("mov %0, %%cr3" : : "r"(phys) : "memory");
}

inline void invlpg(uint64 virt) {
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

inline PageTable* vmmTable(uint64 phys) {
    return (PageTable*)(uintptr)(phys & ADDR_MASK);
}

inline uint64 pml4Index(uint64 v) { return (v >> 39) & ENTRY_MASK; }
inline uint64 pdpIndex (uint64 v) { return (v >> 30) & ENTRY_MASK; }
inline uint64 pdIndex  (uint64 v) { return (v >> 21) & ENTRY_MASK; }
inline uint64 ptIndex  (uint64 v) { return (v >> 12) & ENTRY_MASK; }

inline uint64 vmmAllocZeroPage() {
    uint64 p = allocPage();
    if (!p) return 0;
    uint8* ptr = (uint8*)(uintptr)p;
    for (int i = 0; i < 4096; ++i) ptr[i] = 0;
    return p;
}

inline uint64 vmmSplitHugePage(PageTable* pd, uint64 pdi) {
    uint64 pdEntry = pd->entries[pdi];
    if (!(pdEntry & PTE_HUGE)) {

        return pdEntry & ADDR_MASK;
    }

    uint64 ptPhys = vmmAllocZeroPage();
    if (!ptPhys) return 0;
    PageTable* pt = vmmTable(ptPhys);

    uint64 base = pdEntry & HUGE_MASK;
    uint64 flags = pdEntry & 0xFFF;
    flags &= ~PTE_HUGE;
    for (uint64 i = 0; i < 512; ++i) {
        pt->entries[i] = (base + i * 4096) | flags | PTE_PRESENT;
    }

    pd->entries[pdi] = ptPhys | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    invlpg(base);
    return ptPhys;
}

inline bool mapPage(uint64 virt, uint64 phys, uint64 flags) {
    uint64 cr3 = vmmGetCr3();
    PageTable* pml4 = vmmTable(cr3);

    uint64 pi = pml4Index(virt);
    if (!(pml4->entries[pi] & PTE_PRESENT)) {
        uint64 p = vmmAllocZeroPage();
        if (!p) return false;
        pml4->entries[pi] = p | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    PageTable* pdp = vmmTable(pml4->entries[pi]);

    uint64 di = pdpIndex(virt);
    if (!(pdp->entries[di] & PTE_PRESENT)) {
        uint64 p = vmmAllocZeroPage();
        if (!p) return false;
        pdp->entries[di] = p | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    PageTable* pd = vmmTable(pdp->entries[di]);

    uint64 pdi = pdIndex(virt);
    if (pd->entries[pdi] & PTE_HUGE) {
        uint64 ptPhys = vmmSplitHugePage(pd, pdi);
        if (!ptPhys) return false;
    } else if (!(pd->entries[pdi] & PTE_PRESENT)) {
        uint64 p = vmmAllocZeroPage();
        if (!p) return false;
        pd->entries[pdi] = p | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    PageTable* pt = vmmTable(pd->entries[pdi]);

    uint64 pti = ptIndex(virt);
    pt->entries[pti] = (phys & ADDR_MASK) | flags | PTE_PRESENT;

    invlpg(virt);
    return true;
}

inline void unmapPage(uint64 virt) {
    uint64 cr3 = vmmGetCr3();
    PageTable* pml4 = vmmTable(cr3);

    uint64 pi = pml4Index(virt);
    if (!(pml4->entries[pi] & PTE_PRESENT)) return;
    PageTable* pdp = vmmTable(pml4->entries[pi]);

    uint64 di = pdpIndex(virt);
    if (!(pdp->entries[di] & PTE_PRESENT)) return;
    PageTable* pd = vmmTable(pdp->entries[di]);

    uint64 pdi = pdIndex(virt);
    if (pd->entries[pdi] & PTE_HUGE) {
        // 大页，拆了再解除
        uint64 ptPhys = vmmSplitHugePage(pd, pdi);
        if (!ptPhys) return;
    }
    if (!(pd->entries[pdi] & PTE_PRESENT)) return;
    PageTable* pt = vmmTable(pd->entries[pdi]);

    uint64 pti = ptIndex(virt);
    pt->entries[pti] = 0;

    invlpg(virt);
}

inline uint64 getPhys(uint64 virt) {
    uint64 cr3 = vmmGetCr3();
    PageTable* pml4 = vmmTable(cr3);

    uint64 pi = pml4Index(virt);
    if (!(pml4->entries[pi] & PTE_PRESENT)) return 0;
    PageTable* pdp = vmmTable(pml4->entries[pi]);

    uint64 di = pdpIndex(virt);
    if (!(pdp->entries[di] & PTE_PRESENT)) return 0;
    PageTable* pd = vmmTable(pdp->entries[di]);

    uint64 pdi = pdIndex(virt);
    if (pd->entries[pdi] & PTE_HUGE) {
        uint64 base = pd->entries[pdi] & HUGE_MASK;
        return base + (virt & 0x1FFFFF);
    }
    if (!(pd->entries[pdi] & PTE_PRESENT)) return 0;
    PageTable* pt = vmmTable(pd->entries[pdi]);

    uint64 pti = ptIndex(virt);
    if (!(pt->entries[pti] & PTE_PRESENT)) return 0;

    uint64 base = pt->entries[pti] & ADDR_MASK;
    return base + (virt & 0xFFF);
}

#endif