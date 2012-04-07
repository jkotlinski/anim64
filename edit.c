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

#include "edit.h"

#include <conio.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "anim.h"
#include "anim_play.h"
#include "disk.h"
#include "movie.h"
#include "rle.h"
#include "screen.h"

#pragma codeseg("EDITCODE")
#pragma rodataseg("EDITCODE")

/* animation editor memory layout
 *
 * $6000 - $bfff: screens
 * $c000 - $c7ff: clipboard
 * $c800 - $cfff: RLE buffer
 * $e000 - $ffff: unused
 *
 * note: first plan was to use $8000-$bfff for chars, and switch screen by flipping
 * $d018. this however does not work for 16 screens, since charset is wired to
 * $9000-$9fff.
 */

/* Screens are stored by:
 *  - 40 x 25 character bytes
 *  - 1 packed border/bg color byte
 *  - 40 x 25 bitpacked color nibbles
 */
#define DISPLAY_BASE ((char*)0x400)

static unsigned char cur_x;
static unsigned char cur_y;

static unsigned int offset() {
    return 40 * cur_y + cur_x;
}

static char screen_char() {
    return DISPLAY_BASE[offset()];
}

static char screen_color() {
    return *(char*)(0xd800u + offset());
}

static char hidden_screen_char = ' ';
static char hidden_color;

static void show_cursor() {
    hidden_screen_char = screen_char();
    hidden_color = screen_color();
}

static void punch(char ch, char col) {
    const unsigned int i = offset();
    DISPLAY_BASE[i] = ch;
    *(char*)(0xd800u + i) = col;
}

static char paint_char = 1;
static unsigned char reverse;
static char color = 1;

static void punch_paint() {
    punch(paint_char, color);
}

static void paint(char screen_code) {
    paint_char = screen_code | reverse;
    hidden_screen_char = paint_char;
    hidden_color = color;
    punch_paint();
}

static void hide_cursor() {
    punch(hidden_screen_char, hidden_color);
}

static char has_copy;
void copy_screen() {
    char bg = *(char*)0xd020;
    *(char*)0xd020 = COLOR_GREEN;
    has_copy = 1;
    hide_cursor();
    // Copies one frame.
    memcpy(CLIPBOARD, SCREEN_BASE + SCREEN_SIZE * curr_screen, SCREEN_SIZE);
    *(char*)0xd020 = bg;
}

static void paste_screen() {
    if (!has_copy) return;
    hide_cursor();
    // Copies one frame.
    memcpy(SCREEN_BASE + SCREEN_SIZE * curr_screen, CLIPBOARD, SCREEN_SIZE);
    redraw_edit_screen();
    show_cursor();
}

static void tidy_up_colors() {
    unsigned char prev_color = *(char*)0xd800;
    char* ch = (char*)0x401;
    char* color = (char*)0xd801;
    /* For every space char, use color of previous char. */
    do {
        if (*ch == ' ') {
            *color = prev_color;
        } else {
            prev_color = *color;
        }
        ++color;
        ++ch;
    } while (ch != (char*)0x400 + 40 * 25);
}

void remember_screen() {
    unsigned char* src = (unsigned char*)0xd800;
    unsigned char* dst = curr_screen_colors();
    hide_cursor();
    tidy_up_colors();
    while (src != (unsigned char*)0xd800 + 40 * 25) {
        // Pack nibbles.
        unsigned char packed = *src & 0xf;
        ++src;
        packed |= *src << 4;
        ++src;
        *dst = packed;
        ++dst;
    }
    memcpy(curr_screen_chars(), DISPLAY_BASE, 40 * 25);
    *curr_bg_color() = (*(char*)0xd020 << 4) | (0xf & *(char*)0xd021);
}

static void change_screen(signed char step) {
    remember_screen();
    curr_screen += step;
    if (curr_screen & 0x80u) {
        curr_screen = end_frame;
    } else if (curr_screen > end_frame) {
        curr_screen = 0;
    }
    redraw_edit_screen();
    show_cursor();
}

void switch_color(char c) {
    color = c;
    hidden_color = c;
    punch(hidden_screen_char, c);
}

static void switch_to_console_screen() {
    remember_screen();
    clrscr();
    // *(char*)0xdd00 = 0x17;  // Use graphics bank 0. ($0000-$3fff)
    *(char*)0xd021 = COLOR_BLACK;
    memset((char*)0xd800, COLOR_YELLOW, 0x400);
}

static void load_edit_anim() {
    switch_to_console_screen();
    prompt_open("load", CBM_READ);
    load_and_unpack_anim();
    redraw_edit_screen();
    show_cursor();
}

void inc_d020() {
    ++*(char*)0xd020;
}

#define RLE_BUFFER (char*)0xc800u
#define RLE_BUFFER_SIZE 0x800u

static unsigned int rle_pack_screen() {
    unsigned int packed_bytes = rle_pack(RLE_BUFFER, curr_screen_chars(), SCREEN_SIZE);
    while (packed_bytes > RLE_BUFFER_SIZE) {
        inc_d020();  // Buffer overflow!
    }
    return packed_bytes;
}

static void rle_write_screen() {
    inc_d020();
    if (curr_screen == 0) {
        cbm_write(MY_LFN, RLE_BUFFER, rle_pack_screen());
        return;
    }
    {
        // Interframe compression.
        unsigned int non_iframe_bytes = rle_pack_screen();
        unsigned int iframe_bytes;
        unsigned char use_iframe;
        xor_prev_v2();
        iframe_bytes = rle_pack_screen();

        use_iframe = (iframe_bytes < non_iframe_bytes);
        if (use_iframe) {
            // Write using interframe...
            cbm_write(MY_LFN, RLE_BUFFER, iframe_bytes);
            xor_prev_v2();
        } else {
            // ...un-interframe, repack and write.
            xor_prev_v2();
            cbm_write(MY_LFN, RLE_BUFFER, rle_pack_screen());
        }
        cbm_write(MY_LFN, &use_iframe, 1);
    }
}

static void save_anim() {
    switch_to_console_screen();
    if (prompt_open("save", CBM_WRITE)) {
        const char curr_screen_saved = curr_screen;

        const char version = 2;
        cbm_write(MY_LFN, &version, 1);  // Version.
        ++end_frame;
        cbm_write(MY_LFN, &end_frame, 1);  // Frame count.
        --end_frame;

        for (curr_screen = 0; curr_screen <= end_frame; ++curr_screen) {
            rle_write_screen();
        }
        curr_screen = curr_screen_saved;

        cbm_close(MY_LFN);
        if (_oserror) {
            textcolor(COLOR_RED);
            cputs("disk full?");
            cgetc();
        }
    }
    redraw_edit_screen();
    show_cursor();
    invalidate_loaded_anim();
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

void handle_anim_edit_key(char key) {
    switch (key) {
        default:
            paint(petscii_to_screen(key));
            handle_anim_edit_key(CH_CURS_RIGHT);
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
                handle_anim_edit_key(CH_CURS_DOWN);
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
                handle_anim_edit_key(' ');
                if (!at_right_end) {
                    handle_anim_edit_key(CH_CURS_LEFT);
                }
                handle_anim_edit_key(CH_CURS_LEFT);
            }
            break;
        case CH_F3:  // Change border color.
            inc_d020();
            break;
        case CH_F4:
            ++*(char*)0xd021;
            break;
        case CH_F8:
            ++end_frame;
#define MAX_END_FRAME 15
            end_frame &= MAX_END_FRAME;
            break;
        case ' ':
        case 0x80 | ' ':
            paint(key);
            handle_anim_edit_key(CH_CURS_RIGHT);
            break;
        case CH_STOP:
            remember_screen();
            preview_play_anim(32, 0, 255);
            redraw_edit_screen();
            show_cursor();
            break;

        /* case 0x13:  // HOME
            ++*(VIDEO_BASE + EFFECT_OFFSET);
            *(VIDEO_BASE + EFFECT_OFFSET) %= EFFECT_COUNT;
            break; */
        /* case 0x93:  // CLR (shift + HOME)
            break; */

        case CH_F1: load_edit_anim(); break;
        case CH_F2: invalidate_loaded_anim(); save_anim(); break;
        case CH_F5: copy_screen(); break;
        case CH_F6: paste_screen(); break;
        case CH_F7: switch_to_console_screen(); edit_movie(); redraw_edit_screen(); break;
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

static void anim_edit_loop() {
#define BLINK_PERIOD 30
    int loop = BLINK_PERIOD;

    while (1) {
        unsigned long now = clock();
        while (now == clock()) {}
        if (kbhit()) {
            handle_anim_edit_key(cgetc());
            loop = 0;
        }
        if (loop-- == 0) {
            // Blink.
            punch(screen_char() ^ 0x80, color);
            loop = BLINK_PERIOD;
        }
    }
}

void edit() {
    unsigned char screen;
    textcolor(COLOR_YELLOW);

    for (screen = 0; screen < 16; ++screen) {
        clear_screen(screen);
    }

    anim_edit_loop();
}

