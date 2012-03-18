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

#include "pack.h"
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

#define COLOR_BASE ((char*)0x6000)
#define VIDEO_BASE ((char*)0x8000)
#define COLORS_OFFSET (40 * 25)  // (border << 4) | bg

#define RLE_BUFFER_V1 (unsigned char*)0x6000u
#define RLE_BUFFER_SIZE_V1 0x2000u
#define RLE_BUFFER_V2 (unsigned char*)0xc800u
#define RLE_BUFFER_SIZE_V2 0x800u

unsigned char end_frame = 3;
#define MAX_END_FRAME 15

char* screen_base = VIDEO_BASE;
/* $6000 - $7fff: colors 0-$f
 * $8000 - $bfff: screen 0-$f, + border/screen color
 * $c000 - $c7ff: clipboard
 * $c800 - $cfff: rle buffer
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

static void copy_colors_to_d800() {
    unsigned char* src = COLOR_BASE + curr_screen * 40 * 25 / 2;
    unsigned char* dst = (unsigned char*)0xd800;
    // TODO: Rewrite in assembly.
    while (dst != (unsigned char*)(0xd800 + 40 * 25)) {
        unsigned char colors = *src++;
        *dst++ = colors >> 4;
        *dst++ = colors & 0xf;
    }
}

static void update_screen_base() {
    unsigned char colors;
    screen_base = (char*)(0x8000 + curr_screen * 0x400);
    *(char*)0xd018 = 4 | (curr_screen << 4);  // Point video to 0x8000.
    copy_colors_to_d800();
    colors = screen_base[COLORS_OFFSET];
    *(char*)0xd020 = colors >> 4;
    *(char*)0xd021 = colors & 0xf;
    show_cursor();
}

static void change_screen(char step) {
    remember_colors();
    curr_screen += step;
    if (curr_screen > end_frame) {
        curr_screen = 0;
    } else if (curr_screen < 0) {
        curr_screen = end_frame;
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

static void convert_v1_v2() {
    char screen;
    memmove(COLOR_BASE, VIDEO_BASE + 0x1000, 0x1000);

    // In new version, colors are packed during editing.
    pack_color_nibbles(COLOR_BASE);

    // During rewrite, trash source colors to make non-working colors more obvious...
    // TODO: Remove this call.
    memset(VIDEO_BASE + 0x1000, 0x15, 0x1000);

    // Converts bg/border color bytes.
    for (screen = 0; screen < 4; ++screen) {
        unsigned char* ptr = VIDEO_BASE + screen * 0x400 + COLORS_OFFSET;
        *ptr <<= 4;
        *ptr |= 0xf & *(ptr + 1);
    }
}

static void load_anim() {
    FILE* f;
    switch_to_console_screen();
    f = prompt_open("load", "r");
    if (f) {
        const unsigned char first_byte = fgetc(f);
        switch (first_byte) {
            case 0:
            case 1:
                // Version 1: first_byte is interframe compression on/off.
                fread(RLE_BUFFER_V1, 1, RLE_BUFFER_SIZE_V1, f);
                rle_unpack(VIDEO_BASE, RLE_BUFFER_V1);
                unpack_v1(VIDEO_BASE, first_byte);
                convert_v1_v2();
                break;
            case 2:
                // TODO: Version 2.
            default:
                for (;;) ++*(char*)0xd020;  // Not supported.
        }
        fclose(f);
        curr_screen = 0;
    }
    switch_to_gfx_screen();
}

static void save_anim() {
    FILE* f;
    switch_to_console_screen();
    f = prompt_open("save", "w");
    if (f) {
        fputc(2, f);  // Version.
        fputc(end_frame + 1, f);  // Frame count.

        /*
        unsigned int file_size_interframe_off;
        unsigned int file_size;
        unsigned char use_interframe;
        pack(VIDEO_BASE, 0);
        file_size_interframe_off = rle_pack(&_EDITRAM_LAST__, VIDEO_BASE, SAVE_SIZE);
        unpack(VIDEO_BASE, 0);
        pack(VIDEO_BASE, 1);
        file_size = rle_pack(&_EDITRAM_LAST__, VIDEO_BASE, SAVE_SIZE);
        use_interframe = (file_size <= file_size_interframe_off);  // Interframe byte.
        if (!use_interframe) {
            // Repacks with interframe off.
            unpack(VIDEO_BASE, 1);
            pack(VIDEO_BASE, 0);
            file_size = rle_pack(&_EDITRAM_LAST__, VIDEO_BASE, SAVE_SIZE);
        }

        fputc(use_interframe, f);
        fwrite(&_EDITRAM_LAST__, file_size, 1, f);
        if (EOF == fclose(f)) {
            textcolor(COLOR_RED);
            puts("disk full?");
            cgetc();
        }
        unpack(VIDEO_BASE, use_interframe);
        */
    }
    switch_to_gfx_screen();
    invalidate_loaded_anim();
}

#define CLIPBOARD_CHARS (unsigned char*)0xc000u
#define CLIPBOARD_COLORS (unsigned char*)0xc400u

static char has_copy;
void copy_screen() {
    char bg = *(char*)0xd020;
    *(char*)0xd020 = COLOR_GREEN;
    has_copy = 1;
    hide_cursor();
    // Copies one frame, including colors byte.
    memcpy(CLIPBOARD_CHARS, VIDEO_BASE + 0x400 * curr_screen, COLORS_OFFSET + 1);
    memcpy(CLIPBOARD_COLORS, VIDEO_BASE + 0x1000 + 0x400 * curr_screen, 40 * 25);
    *(char*)0xd020 = bg;
}

static void paste_screen() {
    if (!has_copy) return;
    remember_colors();
    // Copies one frame, including colors byte.
    memcpy(VIDEO_BASE + 0x400 * curr_screen, CLIPBOARD_CHARS, COLORS_OFFSET + 1);
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
        case CH_F3:  // Change border color.
            screen_base[COLORS_OFFSET] += 0x10u;
            *(char*)0xd020 = screen_base[COLORS_OFFSET] >> 4;
            break;
        case CH_F4:
            {
                unsigned char bg = screen_base[COLORS_OFFSET];
                screen_base[COLORS_OFFSET] &= 0xf0u;
                screen_base[COLORS_OFFSET] |= ++bg & 0xf;
                *(char*)0xd021 = bg;
            }
            break;
        case CH_F8:
            ++end_frame;
            end_frame &= MAX_END_FRAME;
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
