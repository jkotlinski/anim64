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
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "diff.h"
#include "disk.h"
// #include "effects.h"
#include "movie.h"
#include "music.h"
#include "rle.h"
#include "player.h"

static unsigned char cur_x;
static unsigned char cur_y;
static unsigned char reverse;

static char color = 1;

#define VIDEO_BASE ((char*)0x8000)
#define BORDER_OFFSET (40 * 25)
#define BG_OFFSET (40 * 25 + 1)
#define END_FRAME (40 * 25 + 2)
#define VERSION (40 * 25 + 3)
#define SAVE_SIZE (0x400 * 4 + 4 * 40 * 25 / 2)

/* The following two are defined by the linker. */
extern unsigned char _EDITRAM_LAST__;

char* screen_base = VIDEO_BASE;
/* RAM end - $7fff: rle buffer
 * $8000 - $8fff: screen 0-3, + border/screen color
 * $9000 - $9fff: colors 0-3
 * $a000 - $cfff: unused
 * $e000 - $ffff: unused
 */

signed char curr_screen;

static void init() {
    clrscr();
    bordercolor(0);
    bgcolor(0);

    memset((void*)0xd800, 0, 0x400);  // Clear colors for better packing.
    *(char*)0xdd00 = 0x15;  // Use graphics bank 2. ($8000-$bfff)
    *(char*)0xd018 = 4;  // Point video to 0x8000.
}

#pragma codeseg("EDITCODE")
#pragma rodataseg("EDITCODE")

static char paint_char = 1;

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

static void hide_cursor() {
    punch(hidden_screen_char, hidden_color);
}

static void show_cursor() {
    hidden_screen_char = screen_char();
    hidden_color = screen_color();
}

static void remember_colors() {
    unsigned int i = 0x1000;
    hide_cursor();
    memcpy(screen_base + 0x1000, (void*)0xd800, 40 * 25);
    // Get rid of noise from reading video area.
    while (i < 0x1000 + 40 * 25) {
        screen_base[i] &= 0xf;
        ++i;
    }
}

static void update_screen_base() {
    screen_base = (char*)(0x8000 + curr_screen * 0x400);
    *(char*)0xd018 = 4 | (curr_screen << 4);  // Point video to 0x8000.
    memcpy((void*)0xd800, screen_base + 0x1000, 40 * 25);
    *(char*)0xd020 = screen_base[BORDER_OFFSET];
    *(char*)0xd021 = screen_base[BG_OFFSET];
    show_cursor();
}

static void change_screen(char step) {
    remember_colors();
    curr_screen += step;
    if (curr_screen > *(VIDEO_BASE + END_FRAME)) {
        curr_screen = 0;
    } else if (curr_screen < 0) {
        curr_screen = *(VIDEO_BASE + END_FRAME);
    }
    update_screen_base();
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

static void paint(char screen_code) {
    paint_char = screen_code | reverse;
    hidden_screen_char = paint_char;
    hidden_color = color;
    punch_paint();
}

void switch_color(char c) {
    color = c;
    hidden_color = c;
    punch(hidden_screen_char, c);
}

static void switch_to_console_screen() {
    remember_colors();
    clrscr();
    *(char*)0xdd00 = 0x17;  // Use graphics bank 0. ($0000-$3fff)
    *(char*)0xd018 = 0x14;  // Point video to 0x400.
    *(char*)0xd021 = COLOR_BLACK;
    memset((char*)0xd800, COLOR_YELLOW, 0x400);
}

static void switch_to_gfx_screen() {
    *(char*)0xdd00 = 0x15;  // Use graphics bank 2. ($8000-$bfff)
    update_screen_base();
}

static void load_anim() {
    FILE* f;
    switch_to_console_screen();
    f = prompt_open("load", "r");
    if (f) {
        fread(&_EDITRAM_LAST__, 1, 0x8000u - (unsigned int)&_EDITRAM_LAST__, f);
        fclose(f);
        rle_unpack(VIDEO_BASE, &_EDITRAM_LAST__);
        undiff(VIDEO_BASE);
        curr_screen = 0;
    }
    switch_to_gfx_screen();
}

static void save_anim() {
    FILE* f;
    switch_to_console_screen();
    f = prompt_open("save", "w");
    if (f) {
        unsigned int file_size;
        VIDEO_BASE[VERSION] = 1;
        diff(VIDEO_BASE);
        file_size = rle_pack(&_EDITRAM_LAST__, VIDEO_BASE, SAVE_SIZE);
        fwrite(&_EDITRAM_LAST__, file_size, 1, f);
        if (EOF == fclose(f)) {
            textcolor(COLOR_RED);
            puts("disk full?");
            cgetc();
        }
        undiff(VIDEO_BASE);
    }
    switch_to_gfx_screen();
    invalidate_loaded_anim();
}

#define CLIPBOARD_CHARS (unsigned char*)0xc000u
#define CLIPBOARD_COLORS (unsigned char*)0xc400u

void copy_screen() {
    hide_cursor();
    // Copies BG_OFFSET + BORDER_OFFSET, excludes END_FRAME.
    memcpy(CLIPBOARD_CHARS, VIDEO_BASE + 0x400 * curr_screen, END_FRAME);
    memcpy(CLIPBOARD_COLORS, VIDEO_BASE + 0x1000 + 0x400 * curr_screen, 40 * 25);
}

void paste_screen() {
    remember_colors();
    // Copies BG_OFFSET + BORDER_OFFSET, excludes END_FRAME.
    memcpy(VIDEO_BASE + 0x400 * curr_screen, CLIPBOARD_CHARS, END_FRAME);
    memcpy(VIDEO_BASE + 0x1000 + 0x400 * curr_screen, CLIPBOARD_COLORS, 40 * 25);
    update_screen_base();
}

static void handle_key(char key) {
    switch (key) {
        default:
            paint(petscii_to_screen(key));
            handle_key(CH_CURS_RIGHT);
            break;
        case CH_CURS_UP:
            if (cur_y > 0) {
                hide_cursor();
                --cur_y;
                show_cursor();
            }
            break;
        case CH_CURS_DOWN:
            if (cur_y < 24) {
                hide_cursor();
                ++cur_y;
                show_cursor();
            }
            break;
        case CH_CURS_LEFT:
            if (cur_x > 0) {
                hide_cursor();
                --cur_x;
                show_cursor();
            }
            break;
        case CH_CURS_RIGHT:
            if (cur_x < 39) {
                hide_cursor();
                ++cur_x;
                show_cursor();
            } else if (cur_y < 24) {
                hide_cursor();
                cur_x = 0;
                show_cursor();
                handle_key(CH_CURS_DOWN);
            }
            break;
        case CH_ENTER:
            change_screen(1);
            break;
        case 0x80 | CH_ENTER:
            change_screen(-1);
            break;
        case CH_DEL:
            {
                const char at_right_end = (cur_x == 39);
                handle_key(' ');
                if (!at_right_end) {
                    handle_key(CH_CURS_LEFT);
                }
                handle_key(CH_CURS_LEFT);
            }
            break;
        case CH_F3:
            *(char*)0xd020 = ++screen_base[BORDER_OFFSET];
            break;
        case CH_F4:
            *(char*)0xd021 = ++screen_base[BG_OFFSET];
            break;
        case CH_F8:
            ++*(VIDEO_BASE + END_FRAME);
            *(VIDEO_BASE + END_FRAME) &= 3;
            break;
        case ' ':
        case 0x80 | ' ':
            paint(key);
            handle_key(CH_CURS_RIGHT);
            break;
        case CH_STOP:
            remember_colors();
            init_music();
            init_play();
            play_anim(32, 0);
            wait_anim(65535u);
            exit_play();
            update_screen_base();
            break;

        /* case 0x13:  // HOME
            ++*(VIDEO_BASE + EFFECT_OFFSET);
            *(VIDEO_BASE + EFFECT_OFFSET) %= EFFECT_COUNT;
            break; */
        /* case 0x93:  // CLR (shift + HOME)
            break; */

        case CH_F1: load_anim(); break;
        case CH_F2: invalidate_loaded_anim(); save_anim(); break;
        case CH_F5: copy_screen(); break;
        case CH_F6: paste_screen(); break;
        case CH_F7: switch_to_console_screen(); edit_movie(); switch_to_gfx_screen(); break;
        case 0x12: reverse = 0x80u; break;
        case 0x92: reverse = 0; break;

        // Colors.
        case 0x05: switch_color(COLOR_WHITE); break;
        case 0x1c: switch_color(COLOR_RED); break;
        case 0x1e: switch_color(COLOR_GREEN); break;
        case 0x1f: switch_color(COLOR_BLUE); break;
        case 0x81: switch_color(COLOR_ORANGE); break;
        case 0x90: switch_color(COLOR_BLACK); break;
        case 0x95: switch_color(COLOR_BROWN); break;
        case 0x96: switch_color(COLOR_LIGHTRED); break;
        case 0x97: switch_color(COLOR_GRAY1); break;
        case 0x98: switch_color(COLOR_GRAY2); break;
        case 0x99: switch_color(COLOR_LIGHTGREEN); break;
        case 0x9a: switch_color(COLOR_LIGHTBLUE); break;
        case 0x9b: switch_color(COLOR_GRAY3); break;
        case 0x9c: switch_color(COLOR_PURPLE); break;
        case 0x9e: switch_color(COLOR_YELLOW); break;
        case 0x9f: switch_color(COLOR_CYAN); break;
    }
}

void edit() {
#define BLINK_PERIOD 30
    int loop = BLINK_PERIOD;

    textcolor(COLOR_YELLOW);

    memset(VIDEO_BASE, 0x20, 0x1000);
    *(VIDEO_BASE + END_FRAME) = 3;
    memset(VIDEO_BASE + 0x1000, 0, 0x1000);

    // Test.
    // handle_key(CH_F8);

    while (1) {
        unsigned long now = clock();
        while (now == clock()) {}
        if (kbhit()) {
            handle_key(cgetc());
            loop = 0;
        }
        if (loop-- == 0) {
            // Blink.
            punch(screen_char() ^ 0x80, color);
            loop = BLINK_PERIOD;
        }
    }
}

#pragma codeseg("CODE")
#pragma rodataseg("CODE")

void main() {
#if 0
    while(1) { if (kbhit()) { printf("%x", cgetc()); } }
#endif
    init();

    play_movie_if_onefiler();

    edit();
}
