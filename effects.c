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

#include "effects.h"

static void flash_colors() {
    static char flash_color;
    unsigned char* ptr = (unsigned char*)0xd800u;
    while (ptr != (unsigned char*)(0xd800u + 40 * 25)) {
        *ptr = flash_color;
        ++ptr;
    }
    ++flash_color;
}

static void shake_x() {
    unsigned int i = 0;
    while (++i < 200) {
        static unsigned char mod;
        *(char*)0xd016u = (*(char*)0xd016u & ~3) | (mod & 3);
        mod += *(char*)0xd012u;
    }
}

void effect_tick(unsigned char anim_screen) {
    char* fx = (char*)(0x8000u + 0x400u * anim_screen + EFFECT_OFFSET);
    /* char fx_param = *(char*)(0x8000u + 0x400u * anim_screen + EFFECT_PARAM_OFFSET); */
    switch (*fx) {
        case FX_FLASH:
            flash_colors();
            break;
        case FX_SHAKE_X:
            shake_x();
            break;
        default:
            // Do nothing.
            break;
    }
}
