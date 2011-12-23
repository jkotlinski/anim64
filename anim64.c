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
#include <_heap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "movie.h"
#include "rle.h"

static unsigned char cur_x;
static unsigned char cur_y;
static unsigned char reverse;

static char color = 1;

#define MUSIC_START ((char*)0x1000)
#define MUSIC_STOP ((char*)0x2800)

#define VIDEO_BASE ((char*)0x8000)
#define BORDER_OFFSET (40 * 25)
#define BG_OFFSET (40 * 25 + 1)
#define ANIM_DELAY_OFFSET (40 * 25 + 2)
#define SAVE_SIZE (0x400 * 7 + 40 * 25)
#define RLE_BUFFER (unsigned char*)0xa000u
char* screen_base = VIDEO_BASE;
/* $8000 - $8fff: screen 0-3, + border/screen color
 * $9000 - $9fff: colors 0-3
 * $a000 - $cfff: rle buffer
 */

char curr_screen;

static char has_music;

static void load_music() {
    FILE* f = fopen("music", "r");
    if (fread(MUSIC_START, 1, MUSIC_STOP - MUSIC_START, f)) {
        has_music = 1;
    }
    fclose(f);
}

static void init_heap() {
    // *(char*)1 &= ~1;  // Switch out BASIC.
    _heaporg = (unsigned*)0xa000;
    _heapptr = (unsigned*)0xa000;
    _heapend = (unsigned*)0xd000;
}

static void init() {
    init_heap();
    clrscr();
    textcolor(COLOR_YELLOW);
    bordercolor(0);
    bgcolor(0);

    load_music();

    memset(VIDEO_BASE, 0x20, 0x1000);
    memset(VIDEO_BASE + 0x1000, 0, 0x1000);
    memset((void*)0xd800, 0, 0x400);  // Clear colors for better packing.
    *(char*)0xdd00 = 0x15;  // Use graphics bank 2. ($8000-$bfff)
    *(char*)0xd018 = 0x04;  // Point video to 0x8000.
}

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
    curr_screen &= 3;
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
    *(char*)0xdd00 = 0x17;  // Use graphics bank 0. ($0000-$3fff)
    *(char*)0xd018 = 0x14;  // Point video to 0x400.
    *(char*)0xd021 = COLOR_BLACK;
    memset((char*)0xd800, COLOR_YELLOW, 0x400);
}

static void switch_to_gfx_screen() {
    *(char*)0xdd00 = 0x15;  // Use graphics bank 2. ($8000-$bfff)
    update_screen_base();
}

static FILE* open(const char* prompt, const char* mode) {
    for (;;) {
        FILE* f;
        char path[32];
        printf("\n%s>", prompt);
        gets(path);
        if (!*path) return NULL;
        f = fopen(path, mode);
        if (f) return f;
        printf("err");
    }
}

static unsigned char anim_delay;

static void load_anim() {
    FILE* f;
    switch_to_console_screen();
    f = open("load", "r");
    if (f) {
        fread(RLE_BUFFER, 1, 0x3000, f);
        fclose(f);
        rle_unpack(VIDEO_BASE, RLE_BUFFER);
        curr_screen = 0;
        anim_delay = VIDEO_BASE[ANIM_DELAY_OFFSET];
    }
    switch_to_gfx_screen();
}

static void save_anim() {
    FILE* f;
    switch_to_console_screen();
    f = open("save", "w");
    if (f) {
        unsigned int file_size;
        VIDEO_BASE[ANIM_DELAY_OFFSET] = anim_delay;
        file_size = rle_pack(RLE_BUFFER, VIDEO_BASE, SAVE_SIZE);
        fwrite(RLE_BUFFER, file_size, 1, f);
        fclose(f);
    }
    switch_to_gfx_screen();
}

/* Defined in colcpy.s. */
void colcpy_9000();
void colcpy_9400();
void colcpy_9800();
void colcpy_9c00();

static unsigned char anim_screen;
static void anim_next_screen() {
    unsigned char* base = (char*)(0x8000 + anim_screen * 0x400);
    *(char*)0xd018 = 4 | (anim_screen << 4);  // Point video to 0x8000.
    *(char*)0xd020 = base[BORDER_OFFSET];
    *(char*)0xd021 = base[BG_OFFSET];
    switch (anim_screen) {
        case 0: colcpy_9000(); break;
        case 1: colcpy_9400(); break;
        case 2: colcpy_9800(); break;
        case 3: colcpy_9c00(); break;
    }
    ++anim_screen;
    anim_screen &= 3;
}

// Defined in music.s.
void init_music();
void tick_music();

static void animate() {
    char keyboard_state = 0;
    char delay = anim_delay;;

    if (has_music) {
        init_music();
    }

    remember_colors();
    anim_screen = 0;

    // Disable kernal timer interrupts.
    *(char*)0xdc0d = 0x7f;
    // Scan all keyboard rows.
    *(char*)0xdc00 = 0;

    for (;;) {
        if (has_music) {
            tick_music();
        }

        // Waits until raster screen is right below lower text border.
        // *(char*)0xd020 = 1;
        while (*(char*)0xd012 != 0xfb) {}
        // *(char*)0xd020 = 0;

        // To exit animation, first all keys should be released, then
        // some key should be pressed.
        if (keyboard_state == 0) {
            if (0xff == *(char*)0xdc01) {  // All keys released?
                keyboard_state = 1;
            }
        } else if (0xff != *(char*)0xdc01) {  // Any key pressed?
            break;
        }
        if (delay-- == 0) {
            anim_next_screen();
            delay = anim_delay;
        }
    }

    // Re-enable kernal timer interrupts.
    *(char*)0xdc0d = 0x81;

    update_screen_base();
}

static void handle_key(char key) {
    switch (key) {
        default:
            paint(petscii_to_screen(key));
            if ((key >= 'a' && key <= 'z') ||
                    (key >= '0' && key <= '9') ||
                    (key == (' ' | 0x80))) {
                handle_key(CH_CURS_RIGHT);
            }
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
                handle_key(' ' | 0x80);
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

        case CH_F1: load_anim(); break;
        case CH_F2: save_anim(); break;
        case CH_F5: ++anim_delay; break;  // Slower.
        case CH_F6: if (anim_delay) --anim_delay; break;  // Faster.
        case CH_F7: animate(); break;
        case CH_F8: switch_to_console_screen(); edit_movie(); switch_to_gfx_screen(); break;
        case 0x12: reverse = 0x80u; break;
        case 0x92: reverse = 0; break;
        case ' ': paint(paint_char); handle_key(CH_CURS_RIGHT); break;

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

void main() {
#define BLINK_PERIOD 30
    int loop = BLINK_PERIOD;
#if 0
    while(1) { if (kbhit()) { printf("%x", cgetc()); } }
#endif
    init();

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
