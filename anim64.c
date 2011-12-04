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
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "keymap.h"

static unsigned char cur_x;
static unsigned char cur_y;

static char color;
static void set_color(char c) {
    color = c;
}

#define VIDEO_BASE (char*)0x8000
char* screen_base = VIDEO_BASE;
/* $8000 - $8fff: screen 0-3
 * $9000 - $9fff: colors 0-3
 */

char curr_screen;

static void init() {
    *(char*)0xdd00 = 0x15;  // Use graphics bank 2. ($8000-$bfff)
    *(char*)0xd018 = 0x04;  // Point video to 0x8000.
    memset(VIDEO_BASE, 0x20, 0x1000);
    memset(VIDEO_BASE + 0x1000, 0, 0x1000);
    cursor(0);
    bordercolor(0);
    set_color(1);
    bgcolor(0);
    init_keymap();
}

#define PAINT_MODE 0
#define KEYMAP_MODE 1
static char mode;

static char color_buffer[40 * 25];

static char last_char = 'a';
static char paint_char = 1;
static char painting;

static unsigned int offset() {
    return 40 * cur_y + cur_x;
}

static void punch(char ch, char col) {
    const unsigned int i = offset();
    screen_base[i] = ch;
    *(char*)(0xd800u + i) = col;
}

static char screen_char() {
    return screen_base[offset()];
}

static char screen_color() {
    return *(char*)(0xd800u + offset());
}

static char hidden_screen_char = ' ';
static char hidden_color;

static void punch_paint() {
    punch(paint_char, color);
}

static void pre_cur_move() {
    if (!painting) {
        punch(hidden_screen_char, hidden_color);
    }
}

static void post_cur_move() {
    hidden_screen_char = painting ? paint_char : screen_char();
    hidden_color = painting ? color : screen_color();
    punch_paint();
}

static void remember_colors() {
    memcpy(screen_base + 0x1000, (void*)0xd800, 40 * 25);
}

static void update_screen_base() {
    curr_screen &= 3;
    screen_base = (char*)(0x8000 + curr_screen * 0x400);
    *(char*)0xd018 = 4 | (curr_screen << 4);  // Point video to 0x8000.
    memcpy((void*)0xd800, screen_base + 0x1000, 40 * 25);
}

static void next_screen() {
    pre_cur_move();
    remember_colors();
    ++curr_screen;
    update_screen_base();
    post_cur_move();
}

static void prev_screen() {
    pre_cur_move();
    remember_colors();
    --curr_screen;
    update_screen_base();
    post_cur_move();
}

static unsigned char petscii_to_screen(unsigned char petscii) {
    if (petscii < 32) {
        return petscii | 128;
    } else if (petscii < 64) {
        return petscii;
    } else if (petscii < 96) {
        return petscii & ~64;
    } else if (petscii < 128) {
        return petscii & ~32;
    } else if (petscii < 160) {
        return petscii | 64;
    } else if (petscii < 192) {
        return petscii - 64;
    } else if (petscii != 255) {
        return petscii & ~128;
    } else {
        return 94;
    }
}

static void do_paint(char ch) {
    if (ch >= '1' && ch <= '8') {  // Textcolor 1-8.
        set_color(ch - '1');
        punch_paint();
    } else if (ch >= '1' - 16 && ch <= '8' - 16) {  // Textcolor 9-16.
        set_color(ch - '1' - 16 + 8);
        punch_paint();
    } else if (ch >= 'a' && ch <= 'z') {  // Paint char.
        if (last_char == ch) {
            hidden_screen_char = paint_char;
            hidden_color = color;
        } else {
            last_char = ch;
            paint_char = get_char(ch);
        }
        punch_paint();
    } else if (ch >= 'A' && ch <= 'Z') {  // Remap char.
        mode = KEYMAP_MODE;
        last_char = ch - 'A' + 'a';
        memcpy(color_buffer, (char*)0xd800, sizeof(color_buffer));
        enter_keymap_mode(ch - 'A');
    } else switch (ch) {
        case CH_CURS_UP:
            if (cur_y > 0) {
                pre_cur_move();
                --cur_y;
                post_cur_move();
            }
            break;
        case CH_CURS_DOWN:
            if (cur_y < 24) {
                pre_cur_move();
                ++cur_y;
                post_cur_move();
            }
            break;
        case CH_CURS_LEFT:
            if (cur_x > 0) {
                pre_cur_move();
                --cur_x;
                post_cur_move();
            }
            break;
        case CH_CURS_RIGHT:
            if (cur_x < 39) {
                pre_cur_move();
                ++cur_x;
                post_cur_move();
            }
            break;
        case ' ':  // "Hold" paint.
            if (painting ^= 1) {
                hidden_screen_char = paint_char;
                hidden_color = color;
                punch_paint();
            }
            bordercolor(painting ? COLOR_RED : COLOR_BLACK);
            break;
        case ',':
            prev_screen();
            break;
        case '.':
            next_screen();
            break;
        case CH_F1:
            *(char*)0xd020 = 5;
            {
                FILE* f = fopen("foo", "r");
                fread(VIDEO_BASE, 0x2000, 1, f);
                curr_screen = 0;
                update_screen_base();
                fclose(f);
            }
            *(char*)0xd020 = 0;
            break;
        case CH_F2:
            *(char*)0xd020 = 4;
            {
                FILE* f = fopen("foo", "w");
                remember_colors();
                fwrite(VIDEO_BASE, 0x2000, 1, f);
                fclose(f);
            }
            *(char*)0xd020 = 0;
    }
}

void main() {
    init();
    punch_paint();
    while (1) {
        if (kbhit()) {
            const char ch = cgetc();
            switch (mode) {
                case PAINT_MODE:
                    do_paint(ch);
                    break;
                case KEYMAP_MODE:
                    if (do_keymap(ch)) {
                        *(char*)0xdd00 = 0x15;  // Use graphics bank 2. ($8000-$bfff)
                        *(char*)0xd018 = 0x04;  // Point video to 0x8000.
                        cursor(0);
                        mode = PAINT_MODE;
                        memcpy((char*)0xd800, color_buffer, sizeof(color_buffer));
                        set_color(color);
                        paint_char = get_char(last_char);
                        punch_paint();
                    }
                    break;
            }
        } else {
            int loop = 1000;
            while (1) {
                unsigned char now = clock();
                while (now == clock()) {}
                if (kbhit()) {
                    break;
                }
                if (--loop == 0) {
                    loop = 1000;
                    punch(screen_char() ^ 0x80, color);
                }
            }
        }
    }
}
