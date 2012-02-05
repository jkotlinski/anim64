/** Copyright (c) 2012, Johan Kotlinski

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

#include "diff.h"

#include "diff_asm.h"

#define END_FRAME (40 * 25 + 2)
#define VERSION (40 * 25 + 3)

#pragma codeseg("LOWCODE")  // Just to make RAM segment smaller.

static void pack_color_nibbles(unsigned char* colorscreen_base) {
    unsigned char screen_it;
    unsigned char* dst = colorscreen_base;
    for (screen_it = 0; screen_it < 4; ++screen_it) {
        unsigned char* src = colorscreen_base + screen_it * 0x400;
        unsigned int i = 0;
        while (i < (40 * 25) / 2) {
            *dst++ = (*src++ << 4) | *src++;
            ++i;
        }
    }
}

// TODO: Write in assembly.
static void unpack_color_nibbles(unsigned char* colorscreen_base) {
    unsigned char screen_it = 3;
    unsigned char* src = colorscreen_base + 4 * 40 * 25 / 2;
    while (1) {
        unsigned char* dst = colorscreen_base + screen_it * 0x400 + 40 * 25;
        unsigned int i = 0;
        while (i < (40 * 25) / 2) {
            const unsigned char val = *--src;
            *--dst = val & 0xf;
            *--dst = val >> 4;
            ++i;
        }
        if (!screen_it--) break;
    }
}

void diff(unsigned char* screen_base) {
    unsigned char screen_it = screen_base[END_FRAME];
    while (screen_it) {
        unsigned char* screen_ptr = screen_base + screen_it * 0x400;
        xor_prev(screen_ptr);  // Characters.
        xor_prev(screen_ptr + 0x1000);  // Colors.
        --screen_it;
    }
    pack_color_nibbles(screen_base + 0x1000);
}

void undiff(unsigned char* screen_base) {
    const unsigned char end_frame = screen_base[END_FRAME];
    unsigned char screen_it = 1;
    if (screen_base[VERSION] != 1) return;
    unpack_color_nibbles(screen_base + 0x1000);
    while (screen_it <= end_frame) {
        unsigned char* screen_ptr = screen_base + screen_it * 0x400;
        xor_prev(screen_ptr);  // Characters.
        xor_prev(screen_ptr + 0x1000);  // Colors.
        ++screen_it;
    }
}
