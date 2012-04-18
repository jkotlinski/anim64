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

#include "anim.h"
#include "convert.h"
#include "player.h"
#include "irq.h"
#include "rle.h"
#include "screen.h"

#include <conio.h>
#include <string.h>

#pragma codeseg("EDITCODE")

static void load_v2_anim(FILE* f) {
    unsigned int read_bytes;
    const unsigned char* rle_start;
    curr_screen = 0;
    fread(&end_frame, 1, 1, f);
    --end_frame;
    // Read all compressed frames to start of screen area...
    read_bytes = fread(SCREEN_BASE, 1, SCREEN_AREA_SIZE, f);
    if (!read_bytes) {
        textcolor(COLOR_RED);
        cputs("err");
        cgetc();
        return;
    }
    // ...move them to end of screen area...
    rle_start = CLIPBOARD - read_bytes;
    memmove(rle_start, SCREEN_BASE, read_bytes);
    // ...then unpack them one by one.
    while (curr_screen <= end_frame) {
        rle_start = rle_unpack(curr_screen_chars(), rle_start);
        if (curr_screen != 0) {
            if (*rle_start) {
                xor_prev_v2();
            }
            ++rle_start;
        }
        ++curr_screen;
    }
}

char load_and_unpack_anim(FILE* f) {
    unsigned char first_byte;
    if (!fread(&first_byte, 1, 1, f)) {
        fclose(f);
        return 0;  // Error.
    }
    switch (first_byte) {
        case 0:
        case 1:
            // Version 1: first_byte is interframe compression on/off.
            convert_v1_v2(first_byte, f);
            break;
        case 2:
            // Version 2.
            load_v2_anim(f);
            break;
        default:
            for (;;) ++*(char*)0xd020;  // Not supported.
    }
    fclose(f);
    curr_screen = 0;
    return 1;
}
