module demo;

extern (C) {
    void vga_clear();
    void vga_putc(char);
    void vga_puts(const(char*));
    void vga_put_int(long);
    void vga_put_hex(ulong, int);
    void vga_set_color(ubyte);
}

extern (C) int d_add(int a, int b) {
    return a + b;
}

extern (C) uint d_fib(uint n) {
    uint a = 0, b = 1;
    foreach (_; 0 .. n) {
        auto t = a + b;
        a = b;
        b = t;
    }
    return a;
}

extern (C) void d_hello() {
    vga_puts("[D] Hello from D module!\n");
}

extern (C) void d_demo(){
    vga_set_color(0x0A);
    vga_puts("[D] ");
    vga_set_color(0x0F);

    vga_puts("d_add(40, 2) = ");
    vga_put_int(d_add(40, 2));
    vga_putc('\n');

    vga_puts("[D] fib(20) = ");
    vga_put_int(d_fib(20));
    vga_putc('\n');

    vga_puts("[D] 0xDEADBEEF = ");
    vga_put_hex(0xDEADBEEF, 8);
    vga_putc('\n');
}