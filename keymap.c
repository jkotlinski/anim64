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

#include "keymap.h"

#include <conio.h>
#include <string.h>

char assign_key;

char keymap['z' - 'a' + 1];
char keymap_index;

char cur_x;
char cur_y;

char reversed;

void update_assign_key(char key) {
    assign_key = key;
    gotoxy(12, 9);
    cputc(assign_key);
    keymap_index = key - 'a';
    cur_x = keymap[keymap_index] & 0xf;
    cur_y = keymap[keymap_index] >> 4;
    gotoxy(cur_x, cur_y);
}

void cursor_moved() {
    keymap[keymap_index] = cur_x | (cur_y << 4);
    gotoxy(cur_x, cur_y);
}

void init_keymap() {
    memset(keymap, 0x48, sizeof(keymap));
}

void draw_keymap() {
    unsigned char x = 0;
    unsigned char y = 0;
    unsigned char ch = reversed ? 0x80 : 0;
    for (y = 0; y < 8; ++y) {
        char* ptr = (char*)(0x400 + y * 40);
        for (x = 0; x < 16; ++x) {
            *ptr = ch;
            ++ptr;
            ++ch;
        }
    }
}

void enter_keymap_mode() {
    clrscr();
    gotoxy(0, 9);
    cputs("assign key:");
    update_assign_key('a');
    draw_keymap();
}

void do_keymap(char key) {
    if (key >= 'a' && key <= 'z') {
        update_assign_key(key);
    } else {
        switch (key) {
            case ' ':
                reversed ^= 1;
                draw_keymap();
                break;
            case CH_CURS_UP:
                if (cur_y > 0) {
                    --cur_y;
                    cursor_moved();
                }
                break;
            case CH_CURS_DOWN:
                if (cur_y < 7) {
                    ++cur_y;
                    cursor_moved();
                }
                break;
            case CH_CURS_LEFT:
                if (cur_x > 0) {
                    --cur_x;
                    cursor_moved();
                }
                break;
            case CH_CURS_RIGHT:
                if (cur_x < 15) {
                    ++cur_x;
                    cursor_moved();
                }
                break;
        }
    }
}
