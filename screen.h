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

#ifndef SCREEN_H_
#define SCREEN_H_

#define SCREEN_BASE ((char*)0x6000)
#define SCREEN_SIZE (40 * 25 + 1 + 40 * 25 / 2)  // 1501 ($5dd) bytes.
#define CLIPBOARD (unsigned char*)0xc000u
#define SCREEN_AREA_SIZE (CLIPBOARD - SCREEN_BASE)
#define BG_COLORS_OFFSET (40 * 25)  // (border << 4) | bg

void clear_screen(char screen);
void xor(char* screen, const char* prev_screen);
void xor_prev_v2();

extern signed char curr_screen;
extern unsigned char end_frame;

unsigned char* curr_screen_chars();
unsigned char* curr_screen_colors();
unsigned char* curr_bg_color();

void redraw_edit_screen();

#endif  // SCREEN_H_
