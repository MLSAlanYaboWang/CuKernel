#if !defined(KEYBOARD_HPP)
#define KEYBOARD_HPP

#include "types.hpp"
#include "io.hpp"

static bool keyboardShift = false;
static bool keyboardCaps  = false;

static const int32 KEYBOARD_BUFFER_SIZE = 256;
inline volatile cchar keyboardBuffer[KEYBOARD_BUFFER_SIZE];
inline volatile int32 keyboardHead = 0;
inline volatile int32 keyboardTail = 0;

inline void keyboardBufferPut(cchar c) {
    int32 next = (keyboardHead + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != keyboardTail) {
        keyboardBuffer[keyboardHead] = c;
        keyboardHead = next;
    }
}

inline bool keyboardBufferGet(cchar* out) {
    if (keyboardHead == keyboardTail) return false;
    *out = keyboardBuffer[keyboardTail];
    keyboardTail = (keyboardTail + 1) % KEYBOARD_BUFFER_SIZE;
    return true;
}

static const cchar keyboardMapNormal[128] = {
    // 0x00
    0,    27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    // 0x10
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,  'a', 's',
    // 0x20
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,  '\\','z', 'x', 'c', 'v',
    // 0x30
    'b', 'n', 'm', ',', '.', '/', 0,  '*', 0,  ' ', 0,  0,  0,  0,  0,  0,
    // 0x40
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    // 0x50
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    // 0x60
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    // 0x70
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static const cchar keyboardMapShift[128] = {
    // 0x00
    0,    27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    // 0x10
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,  'A', 'S',
    // 0x20
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,  '|', 'Z', 'X', 'C', 'V',
    // 0x30
    'B', 'N', 'M', '<', '>', '?', 0,  '*', 0,  ' ', 0,  0,  0,  0,  0,  0,
    // 0x40
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    // 0x50
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    // 0x60
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    // 0x70
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

inline cchar keyboardHandle(uint8 scanCode) {
    if (scanCode & 0x80) {
        uint8 make = scanCode & 0x7F;
        if ((make == 0x2A) | (make == 0x36)) keyboardShift = false;
        return 0;
    }
    switch (scanCode) {
        case 0x2A: case 0x36:
            keyboardShift = true;
            return 0;
        case 0x3A:
            keyboardCaps = !keyboardCaps;
            return 0;
        default:
            break;
    }

    cchar c;
    bool isLetter = (scanCode >= 0x10 && scanCode <= 0x19) ||
                    (scanCode >= 0x1E && scanCode <= 0x26) ||
                    (scanCode >= 0x2C && scanCode <= 0x32);
    
    bool upper = keyboardShift;
    if (isLetter) upper = keyboardShift ^ keyboardCaps;

    c = upper ? keyboardMapShift[scanCode] : keyboardMapNormal[scanCode];
    return c;
}

#endif