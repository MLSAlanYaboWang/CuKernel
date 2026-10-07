#include "../head/amd64/VGA_C_API.hpp"

static uint8 currentAttr = 0x0F;

extern "C" void vga_clear() { clearScreen(); }

extern "C" void vga_putc(char c) { printChar(c); }

extern "C" void vga_puts(const char* s) { printString(s); }

extern "C" void vga_put_int(long long n) { printInt(n); }

extern "C" void vga_put_hex(unsigned long long v, int digits) {
    switch (digits) {
        case 1:  printHex<1>(v);  break;
        case 2:  printHex<2>(v);  break;
        case 4:  printHex<4>(v);  break;
        case 8:  printHex<8>(v);  break;
        case 16: printHex<16>(v); break;
        default: {
            const char* hex = "0123456789ABCDEF";
            char buf[2 + 16 + 1];
            int d = digits < 1 ? 1 : (digits > 16 ? 16 : digits);
            buf[0] = '0'; buf[1] = 'x';
            for (int i = 0; i < d; ++i)
                buf[2 + i] = hex[(v >> ((d - 1 - i) * 4)) & 0xF];
            buf[2 + d] = 0;
            printString(buf);
        }
    }
}

extern "C" void vga_set_color(unsigned char attr) { currentAttr = attr; }