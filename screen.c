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

#include "screen.h"

#include <string.h>

#include "loops.h"

signed char curr_screen;
unsigned char end_frame = 3;

#define SCREEN_COLORS_OFFSET (40 * 25 + 1)

unsigned char* curr_screen_chars() {
    return SCREEN_BASE + curr_screen * SCREEN_SIZE;
}

unsigned char* curr_screen_colors() {
    return curr_screen_chars() + SCREEN_COLORS_OFFSET;
}

unsigned char* curr_bg_color() {
    return curr_screen_chars() + BG_COLORS_OFFSET;
}

void redraw_edit_screen() {
    unsigned char colors;
    memcpy((char*)0x400, curr_screen_chars(), 40 * 25);
    copy_colors_to_d800(curr_screen_colors());
    colors = *curr_bg_color();
    *(char*)0xd021 = colors;
    *(char*)0xd020 = colors >> 4;
}

void clear_screen(char screen) {
    char* ptr = SCREEN_BASE + screen * SCREEN_SIZE;
    memset(ptr, 0x20, SCREEN_SIZE);
    ptr[BG_COLORS_OFFSET] = 0;
}

void xor_prev_v2() {
    char* curr = curr_screen_chars();
    xor_v2(curr, curr - SCREEN_SIZE);
}

