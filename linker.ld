ENTRY(kernelMain)

SECTIONS
{
    . = 0x10000;

    .text : {
        *(.text.kernelMain)
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        _bssStart = .;
        *(.bss)
        *(COMMON)
        _bssEnd = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}