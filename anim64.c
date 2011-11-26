/** Copyright (c) 2011, Johan Kotlinski

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

#include <conio.h>

#include "keymap.h"

#define SCREEN (char*)0x400u

static char cur_x;
static char cur_y;

static char color;
static void set_color(char c) {
    textcolor(c);
    color = c;
}

static void init() {
    clrscr();
    bordercolor(0);
    set_color(1);
    bgcolor(0);
    cursor(1);
    init_keymap();
}

#define PAINT_MODE 0
#define KEYMAP_MODE 1
char mode;

static void move_cursor() {
    gotoxy(cur_x, cur_y);
}

void do_paint(char ch) {
    if (ch >= '1' && ch <= '8') {  // textcolor 1-8
        set_color(ch - '1');
    } else if (ch >= '1' - 16 && ch <= '8' - 16) {  // textcolor 9-16
        set_color(ch - '1' - 16 + 8);
    } else if (ch >= 'a' && ch <= 'z') {
        unsigned int offset = 40 * cur_y + cur_x;
        *(char*)(0x400u + offset) = get_char(ch);
        *(char*)(0xd800u + offset) = color;
    } else if (ch >= 'A' && ch <= 'Z') {
        mode = KEYMAP_MODE;
        enter_keymap_mode(ch - 'A');
    } else switch (ch) {
        case CH_CURS_UP:
            if (cur_y > 0) {
                --cur_y;
                move_cursor();
            }
            break;
        case CH_CURS_DOWN:
            if (cur_y < 24) {
                ++cur_y;
                move_cursor();
            }
            break;
        case CH_CURS_LEFT:
            if (cur_x > 0) {
                --cur_x;
                move_cursor();
            }
            break;
        case CH_CURS_RIGHT:
            if (cur_x < 39) {
                ++cur_x;
                move_cursor();
            }
            break;
    }
}

void main() {
    init();
    while (1) {
        const char ch = cgetc();
        switch (mode) {
            case PAINT_MODE:
                do_paint(ch);
                break;
            case KEYMAP_MODE:
                if (do_keymap(ch)) {
                    mode = PAINT_MODE;
                    clrscr();
                    move_cursor();
                }
                break;
        }
    }
}
