[BITS 16]
[ORG 0x7C00]

KERNEL_OFFSET   equ 0x10000
KERNEL_TMP      equ 0x7E00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [bootDrive], dl
    mov si, msgLoading
    call printString

    call doE820
    jc   errExit

    call enableA20

    mov ax, 0x1000
    mov es, ax
    xor di, di
    mov si, KERNEL_TMP
    mov cx, KERNEL_SECTORS
    shl cx, 8
    cld
    rep movsw

    xor ax, ax
    mov es, ax

    call switchToPm
    mov ax, 0x4F01
    mov cx, 0x118
    mov di, 0x5000
    int 0x10
    cmp ax, 0x004F
    jne errExit

    mov eax, [0x5000 + 0x28]
    mov [0x6000], eax
    mov ax, [0x5000 + 0x12]
    mov [0x6004], ax
    mov ax, [0x5000 + 0x14]
    mov [0x6006], ax
    mov al, [0x5000 + 0x19]
    mov [0x6008], al
    mov ax, [0x5000 + 0x10]
    mov [0x600A], ax

    mov ax, 0x4F02
    mov bx, 0x4118
    int 0x10
    cmp ax, 0x004F
    jne errExit
    jmp $

errExit:
    mov si, msgErr
    call printString
    jmp $
    
printString:
    mov ah, 0x0E
.next:
    lodsb
    or  al, al
    jz  .done
    int 0x10
    jmp .next
.done:
    ret

enableA20:
    in  al, 0x92
    or  al, 2
    and al, 0xFE
    out 0x92, al
    ret

doE820:
    pusha
    push es
    xor ax, ax
    mov es, ax
    mov di, 0x5004
    xor ebx, ebx
    xor bp, bp
.next:
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, 24
    int 0x15
    jc  .done
    cmp eax, 0x534D4150
    jne .fail
    test ebx, ebx
    jz  .last
    inc bp
    add di, 24
    jmp .next
.last:
    inc bp
.done:
    mov dword [0x5000], ebp
    clc
    pop es
    popa
    ret
.fail:
    stc
    pop es
    popa
    ret

switchToPm:
    cli
    lgdt [gdt32Descriptor]

    mov eax, cr0
    or  eax, 0x1
    mov cr0, eax
    jmp CODE32_SEG:initPm32

[BITS 32]
initPm32:
    mov ax, DATA32_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebp, 0x90000
    mov esp, ebp

    ; 页表
    mov dword [0x1000], 0x2000 | 0x03
    mov dword [0x1004], 0
    mov dword [0x2000], 0x3000 | 0x03
    mov dword [0x2004], 0

    mov edi, 0x3000
    mov eax, 0x83
    mov ecx, 512
.fillPd:
    mov [edi], eax
    mov dword [edi+4], 0
    add eax, 0x200000
    add edi, 8
    loop .fillPd

    mov eax, 0x1000
    mov cr3, eax

    mov eax, cr4
    or  eax, (1 << 5)
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or  eax, (1 << 8)
    wrmsr

    mov eax, cr0
    or  eax, (1 << 31)
    mov cr0, eax

    lgdt [gdt64Descriptor]
    jmp CODE64_SEG:initPm64

[BITS 64]
initPm64:
    mov ax, DATA64_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rsp, 0x90000

    mov rax, KERNEL_OFFSET
    call rax

    jmp $

gdt32Start:
    dq 0
gdt32Code:
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00
gdt32Data:
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00
gdt32End:
gdt32Descriptor:
    dw gdt32End - gdt32Start - 1
    dd gdt32Start
CODE32_SEG equ gdt32Code - gdt32Start
DATA32_SEG equ gdt32Data - gdt32Start

gdt64Start:
    dq 0
gdt64Code:
    dw 0x0000, 0x0000
    db 0x00, 0x9A, 0x20, 0x00
gdt64Data:
    dw 0x0000, 0x0000
    db 0x00, 0x92, 0x00, 0x00
gdt64End:
gdt64Descriptor:
    dw gdt64End - gdt64Start - 1
    dq gdt64Start
CODE64_SEG equ gdt64Code - gdt64Start
DATA64_SEG equ gdt64Data - gdt64Start

bootDrive  db 0
msgLoading db "Loading...", 13, 10, 0
msgErr     db "Boot error!", 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55