#if !defined(VGA_CONSOLE_HPP)
#define VGA_CONSOLE_HPP

#include "types.hpp"

static volatile uint8* const vgaBuffer = (uint8*)0xB8000;
static const int32 screenWidth = 80;
static const int32 screenHeight = 25;
static const int32 maxCol = screenWidth - 1;
static const int32 maxRow = screenHeight - 1;

static int32 cursorRow = 0;
static int32 cursorCol = 0;

static void clearScreen(void) {
    for (int32 i = 0; i < screenWidth * screenHeight; ++i) {
        vgaBuffer[i * 2] = ' ';
        vgaBuffer[i * 2 + 1] = 0x07;
    }
    cursorRow = 0;
    cursorCol = 0;
}

static void scrollScreen(int lines) {
    if (lines == 0) return;

    if (lines >= screenHeight || lines <= -screenHeight) {
        clearScreen();
        return;
    }

    if (lines > 0) {
        for (int row = lines; row < screenHeight; ++row) {
            for (int col = 0; col < screenWidth; ++col) {
                int src = (row * screenWidth + col) * 2;
                int dst = ((row - lines) * screenWidth + col) * 2;
                vgaBuffer[dst]     = vgaBuffer[src];
                vgaBuffer[dst + 1] = vgaBuffer[src + 1];
            }
        }
        for (int row = screenHeight - lines; row < screenHeight; ++row) {
            for (int col = 0; col < screenWidth; ++col) {
                int idx = (row * screenWidth + col) * 2;
                vgaBuffer[idx]     = ' ';
                vgaBuffer[idx + 1] = 0x07;
            }
        }
        cursorRow -= lines;
        if (cursorRow < 0) cursorRow = 0;
    } else {
        int n = -lines;
        for (int row = screenHeight - 1; row >= n; --row) {
            for (int col = 0; col < screenWidth; ++col) {
                int src = ((row - n) * screenWidth + col) * 2;
                int dst = (row * screenWidth + col) * 2;
                vgaBuffer[dst]     = vgaBuffer[src];
                vgaBuffer[dst + 1] = vgaBuffer[src + 1];
            }
        }
        for (int row = 0; row < n; ++row) {
            for (int col = 0; col < screenWidth; ++col) {
                int idx = (row * screenWidth + col) * 2;
                vgaBuffer[idx]     = ' ';
                vgaBuffer[idx + 1] = 0x07;
            }
        }
        cursorRow += n;
        if (cursorRow >= screenHeight) cursorRow = screenHeight - 1;
    }
}

static void printChar(cchar c) {
    if (c == '\n') {
        cursorCol = 0;
        ++cursorRow;
    }
    else if (c == '\r') {
        cursorCol = 0;
    }
    else if (c == '\b') {
        if (cursorCol == 0) {
            if (cursorRow == 0) return;
            --cursorRow;
            cursorCol = maxCol;
        }
        else {
            --cursorCol;
            int32 idx = (cursorRow * screenWidth + cursorCol) * 2;
            vgaBuffer[idx] = ' ';
        }
    }
    else if (c == '\t') {
        int32 next = (cursorCol + 4) & ~3;
        while (cursorCol < next && cursorCol < screenWidth) {
            printChar(' ');
        }
    }
    else {
        int32 idx = (cursorRow * screenWidth + cursorCol) * 2;
        vgaBuffer[idx] = c;
        vgaBuffer[idx + 1] = 0x0F;
        if (++cursorCol >= screenWidth) {
            cursorCol = 0;
            ++cursorRow;
        }
    }
    if (cursorRow >= screenHeight) {
        scrollScreen(1);
    }
}

static void printString(cstring s) {
    while (*s) printChar(*s++);
}

static void printInt(int64 n) {
    if (n == 0) {
        printChar('0');
        return;
    }
    uint64 u;
    if (n < 0) {
        printChar('-');
        u = (uint64)(-(n + 1)) + 1;
    }
    else {
        u = (uint64)n;
    }
    cchar buf[20];
    int32 i = 0;
    while (u > 0) {
        buf[i++] = '0' + (u % 10);
        u /= 10;
    }
    while (i > 0) {
        printChar(buf[--i]);
    }
}

template<int Digits>
static void printHex(uint64 v) {
    static_assert(Digits >= 1 && Digits <= 16, "Digits must be 1..16");
    cchar buf[2 + 16 + 1];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < Digits; ++i) {
        int nib = (v >> ((Digits - 1 - i) * 4)) & 0xF;
        buf[2 + i] = nib < 10 ? '0' + nib : 'A' + nib - 10;
    }
    buf[2 + Digits] = 0;
    printString(buf);
}

#endif