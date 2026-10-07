#if !defined(VGA_C_API_HPP)
#define VGA_C_API_HPP

#include "VGA_console.hpp"

extern "C" {
    void vga_clear();
    void vga_putc(char c);
    void vga_puts(const char* s);
    void vga_put_int(long long n);
    void vga_put_hex(unsigned long long v, int digits);
    void vga_set_color(unsigned char attr);
}

#endif