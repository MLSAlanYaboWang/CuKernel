#if !defined(HEAP_HPP)
#define HEAP_HPP

#include "types.hpp"
#include "vmm.hpp"
#include "pmm.hpp"

static const uint64 HEAP_BASE = 0x10000000ULL;
static const uint64 HEAP_SIZE = 0x1000000ULL;

static const uint64 MIN_BLOCK = 32;

static const uint64 ALIGN = 16;

struct BlockHeader {
    uint64 size;
    uint64 free; 
    BlockHeader* next;
    BlockHeader* prev;
};

static BlockHeader* heapHead = nullptr;

inline uint64 alignUp(uint64 x) {
    return (x + ALIGN - 1) & ~(ALIGN - 1);
}

inline void heapInit() {
    for (uint64 addr = HEAP_BASE; addr < HEAP_BASE + HEAP_SIZE; addr += 4096) {
        uint64 phys = allocPage();
        if (!phys) return;
        mapPage(addr, phys, PTE_PRESENT | PTE_WRITABLE);
    }

    heapHead = (BlockHeader*)(uintptr)HEAP_BASE;
    heapHead->size = HEAP_SIZE;
    heapHead->free = 1;
    heapHead->next = nullptr;
    heapHead->prev = nullptr;
}

// First Fit Algorithm
inline void* kmalloc(uint64 size) {
    if (size == 0) return nullptr;

    uint64 need = alignUp(size) + sizeof(BlockHeader);
    if (need < MIN_BLOCK) need = MIN_BLOCK;

    BlockHeader* cur = heapHead;
    while (cur) {
        if (cur->free && cur->size >= need) {
            if (cur->size >= need + MIN_BLOCK) {
                BlockHeader* newBlock = (BlockHeader*)((uintptr)cur + need);
                newBlock->size = cur->size - need;
                newBlock->free = 1;
                newBlock->next = cur->next;
                newBlock->prev = cur;

                if (cur->next) cur->next->prev = newBlock;
                cur->next = newBlock;
                cur->size = need;
            }
            cur->free = 0;
            return (void*)((uintptr)cur + sizeof(BlockHeader));
        }
        cur = cur->next;
    }
    return nullptr;
}

inline void kfree(void* p) {
    if (!p) return;

    BlockHeader* b = (BlockHeader*)((uintptr)p - sizeof(BlockHeader));
    b->free = 1;

    if (b->next && b->next->free) {
        b->size += b->next->size;
        b->next = b->next->next;
        if (b->next) b->next->prev = b;
    }

    if (b->prev && b->prev->free) {
        b->prev->size += b->size;
        b->prev->next = b->next;
        if (b->next) b->next->prev = b->prev;
    }
}

#endif