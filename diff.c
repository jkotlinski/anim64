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

#define END_FRAME (40 * 25 + 2)

static void xor_prev(unsigned char* screen_ptr) {
    unsigned int offset = 0;
    unsigned char* prev_ptr = screen_ptr - 0x400;
    while (offset < 40 * 25) {
        *screen_ptr++ ^= *prev_ptr++;
        ++offset;
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
}

void undiff(unsigned char* screen_base) {
    const unsigned char end_frame = screen_base[END_FRAME];
    unsigned char screen_it = 1;
    while (screen_it <= end_frame) {
        unsigned char* screen_ptr = screen_base + screen_it * 0x400;
        xor_prev(screen_ptr);  // Characters.
        xor_prev(screen_ptr + 0x1000);  // Colors.
        ++screen_it;
    }
}
