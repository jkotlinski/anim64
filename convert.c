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

#include "convert.h"

#include <string.h>

#include "loops.h"
#include "rle.h"
#include "screen.h"

#pragma codeseg("EDITCODE")
#pragma rodataseg("EDITCODE")

#define END_FRAME (40 * 25 + 2)
#define VERSION (40 * 25 + 3)

static void unpack_color_nibbles(unsigned char* colorscreen_base) {
    unsigned char screen_it = 3;
    unsigned char* src = colorscreen_base + 4 * 40 * 25 / 2;
    while (1) {
        unsigned char* dst = colorscreen_base + screen_it * 0x400 + 40 * 25;
        ++*(char*)0xd020;
        while (1) {
            const unsigned char val = *--src;
            *--dst = val;
            *--dst = val >> 4;
            if (!((unsigned int)dst & 0x3ffu)) break;
        }
        if (!screen_it--) break;
    }
}

static void unpack_v1(unsigned char* screen_base, char interframe_compression) {
    const unsigned char end_frame = screen_base[END_FRAME];
    unsigned char screen_it = 1;
    if (screen_base[VERSION] != 1) return;
    unpack_color_nibbles(screen_base + 0x1000);
    if (!interframe_compression)
        return;
    while (screen_it <= end_frame) {
        unsigned char* screen_ptr = screen_base + screen_it * 0x400;
        xor_prev(screen_ptr);  // Characters.
        xor_prev(screen_ptr + 0x1000);  // Colors.
        ++screen_it;
        ++*(char*)0xd020;
    }
}

void inc_d020();

void convert_v1_v2(char use_iframe, FILE* f) {
    char screen;
    fread((char*)0x8000, 1, 0x2000, f);
    rle_unpack((char*)0xa000u, (char*)0x8000u);
    unpack_v1((char*)0xa000u, use_iframe);

    for (screen = 0; screen < 4; ++screen) {
        // Move chars.
        unsigned char* src = (char*)0xa000u + 0x400u * screen;
        unsigned char* dst = SCREEN_BASE + SCREEN_SIZE * screen;
        unsigned int i;
        inc_d020();
        memcpy(dst, src, 40 * 25);
        // Border + bg colors.
        dst += 40 * 25;
        src += 40 * 25;
        *dst = (*src << 4) | (src[1] & 0xf);
        // Move & pack color nibbles.
        ++dst;
        src = (char*)0xb000u + 0x400u * screen;
        for (i = 0; i < 40 * 25; ++i) {
            *dst = (*src & 0xf) | (src[1] << 4);
            ++dst;
            src += 2;
        }
    }
    // Clean temp areas.
    for (; screen < 16; ++screen) {
        clear_screen(screen);
    }
}
