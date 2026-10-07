#if !defined(INCLUDE_HPP)
#define INCLUDE_HPP

#if defined(AMD64)
    #include "amd64/types.hpp"
    #include "amd64/VGA_console.hpp"
    #include "amd64/idt.hpp"
    #include "amd64/io.hpp"
    #include "amd64/regs.hpp"
    #include "amd64/keyboard.hpp"
    #include "amd64/e820.hpp"    // Unavailable
    #include "amd64/pmm.hpp"
    #include "amd64/vmm.hpp"
    #include "amd64/heap.hpp"
    #include "amd64/ata.hpp"    // Unavailable
    #include "amd64/ahci.hpp"
    #include "amd64/pci.hpp"
    #include "amd64/fat32.hpp"
    #include "amd64/VGA_C_API.hpp"
#endif

#endif