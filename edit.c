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
#include "lz77.h"
#include "screen.h"
#include "cc65/mycbm.h"

#pragma codeseg("EDITCODE")
#pragma rodataseg("EDITCODE")

/* animation editor memory layout
 *
 * $6000 - $bfff: screens
 * $c000 - $c7ff: clipboard
 * $c800 - $cfff: RLE buffer
 * $d000 - $dfff: VIC
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

/* (CLIP_X1, CLIP_Y1) = top left.
 * (CLIP_X2, CLIP_Y2) = bottom right.
 */
static char CLIP_X1 = 0xff;
static char CLIP_X2;
static char CLIP_Y1;
static char CLIP_Y2;

static void paint_copy_mark() {
    const unsigned char x2 = ((CLIP_X1 < CLIP_X2) ? CLIP_X2 : CLIP_X1) + 1;
    const unsigned char y2 = ((CLIP_Y1 < CLIP_Y2) ? CLIP_Y2 : CLIP_Y1) + 1;
    unsigned char y1 = (CLIP_Y1 < CLIP_Y2) ? CLIP_Y1 : CLIP_Y2;
    const char alt_color = 0xf ^ *(char*)0xd020;
    while (y1 < y2) {
        unsigned char x1 = (CLIP_X1 < CLIP_X2) ? CLIP_X1 : CLIP_X2;
        while (x1 < x2) {
            const unsigned int offs = y1 * 40 + x1;
            DISPLAY_BASE[offs] ^= 0x80;
            ((char*)0xd800)[offs] = alt_color;
            ++x1;
        }
        ++y1;
    }
}

static void copy() {
    remember_screen();  // Saves edit screen to SCREEN_BASE area.

    CLIP_X1 = cur_x;
    CLIP_X2 = cur_x;
    CLIP_Y1 = cur_y;
    CLIP_Y2 = cur_y;

    for (;;) {
        redraw_edit_screen();
        paint_copy_mark();
poll_key:
        switch (cgetc()) {
            case CH_CURS_DOWN:
                if (CLIP_Y2 < 24) ++CLIP_Y2;
                break;
            case CH_CURS_UP:
                if (CLIP_Y2) --CLIP_Y2;
                break;
            case CH_CURS_RIGHT:
                if (CLIP_X2 < 39) ++CLIP_X2;
                break;
            case CH_CURS_LEFT:
                if (CLIP_X2) --CLIP_X2;
                break;
            case CH_F5:
                goto done;
            default:
                goto poll_key;
        }
    }

done:
    redraw_edit_screen();
    memcpy(CLIPBOARD, DISPLAY_BASE, 40 * 25);
    memcpy(CLIPBOARD + 40 * 25, (char*)0xd800, 40 * 25);

    if (CLIP_X1 == CLIP_X2 && CLIP_Y1 == CLIP_Y2) {
        // Copies entire screen. Flashes screen to give some kind of feedback.
        *(char*)0xd021 ^= 0xf;
        {
            unsigned long now = clock();
            while (now + 8 != clock()) {}
        }
        *(char*)0xd021 ^= 0xf;
    } else {
        // Order coordinates.
        if (CLIP_X1 > CLIP_X2) {
            const char tmp = CLIP_X1;
            CLIP_X1 = CLIP_X2;
            CLIP_X2 = tmp;
        }
        if (CLIP_Y1 > CLIP_Y2) {
            const char tmp = CLIP_Y1;
            CLIP_Y1 = CLIP_Y2;
            CLIP_Y2 = tmp;
        }
    }
}

static void paste() {
    if (CLIP_X1 == 0xff) return;

    if (CLIP_X1 == CLIP_X2 && CLIP_Y1 == CLIP_Y2) {
        // Copies entire screen.
        memcpy(DISPLAY_BASE, CLIPBOARD, 40 * 25);
        memcpy((char*)0xd800, CLIPBOARD + 40 * 25, 40 * 25);
    } else {
        // Pastes region.
        char y;
        for (y = CLIP_Y1; y <= CLIP_Y2; ++y) {
            const char dst_y = y + cur_y - CLIP_Y1;
            char x;
            if (dst_y >= 25) break;
            for (x = CLIP_X1; x <= CLIP_X2; ++x) {
                const char dst_x = x + cur_x - CLIP_X1;
                if (dst_x >= 40) break;
                DISPLAY_BASE[dst_y * 40 + dst_x] = CLIPBOARD[y * 40 + x];
                ((char*)0xd800)[dst_y * 40 + dst_x] = CLIPBOARD[40 * 25 + y * 40 + x];
            }
        }
    }

    show_cursor();
    remember_screen();
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
    memset((char*)0xd800, COLOR_YELLOW, 40 * 25);
}

static void import_linde() {
    switch_to_console_screen();
    if (prompt_open("import", CBM_READ, TYPE_PRG)) {
        if (!load_linde()) {
            cputs("err");
        }
    }
    show_cursor();
}

static void load_edit_anim() {
    switch_to_console_screen();
    if (prompt_open("load", CBM_READ, TYPE_USR)) {
        load_and_unpack_anim();
    }
    redraw_edit_screen();
    show_cursor();
}

void inc_d020() {
    ++*(char*)0xd020;
}

#define PACK_BUFFER (char*)0xc800u
#define PACK_BUFFER_SIZE 0x800u

static unsigned int lz77_pack_screen() {
    unsigned int packed_bytes = lz77_pack(PACK_BUFFER, curr_screen_chars());
    while (packed_bytes > PACK_BUFFER_SIZE) {
        inc_d020();  // Buffer overflow!
    }
    return packed_bytes;
}

static void lz77_write_screen() {
    inc_d020();
    if (curr_screen == 0) {
        mycbm_write(MY_LFN, PACK_BUFFER, lz77_pack_screen());
        return;
    }
    {
        // Interframe compression.
        unsigned int non_iframe_bytes = lz77_pack_screen();
        unsigned int iframe_bytes;
        unsigned char use_iframe;
        xor_prev_v2();
        iframe_bytes = lz77_pack_screen();

        use_iframe = (iframe_bytes < non_iframe_bytes);
        if (use_iframe) {
            // Write using interframe...
            mycbm_write(MY_LFN, PACK_BUFFER, iframe_bytes);
            xor_prev_v2();
        } else {
            // ...un-interframe, repack and write.
            xor_prev_v2();
            mycbm_write(MY_LFN, PACK_BUFFER, lz77_pack_screen());
        }
        mycbm_write(MY_LFN, &use_iframe, 1);
    }
}

static void save_anim() {
    switch_to_console_screen();
    if (prompt_open("save", CBM_WRITE, TYPE_USR)) {
        const char curr_screen_saved = curr_screen;

        const char version = 3;
        mycbm_write(MY_LFN, &version, 1);  // Version.
        ++end_frame;
        mycbm_write(MY_LFN, &end_frame, 1);  // Frame count.
        --end_frame;

        for (curr_screen = 0; curr_screen <= end_frame; ++curr_screen) {
            lz77_write_screen();
        }
        curr_screen = curr_screen_saved;

        mycbm_close(MY_LFN);
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
            hide_cursor();
            if (cur_x) {
                --cur_x;
            } else {
                cur_x = 39;
                if (cur_y) --cur_y;
            }
            show_cursor();
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
            switch_to_console_screen();
            cputs("frames (1-16): ");
            end_frame = (read_digits() - 1) & 15;
            redraw_edit_screen();
            show_cursor();
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

        case 0x93:  // CLR (shift + HOME)
            import_linde();
            break;

        /* case 0x13:  // HOME
            ++*(VIDEO_BASE + EFFECT_OFFSET);
            *(VIDEO_BASE + EFFECT_OFFSET) %= EFFECT_COUNT;
            break; */
        /* case 0x93:  // CLR (shift + HOME)
            break; */

        case CH_F1: load_edit_anim(); break;
        case CH_F2: invalidate_loaded_anim(); save_anim(); break;
        case CH_F5: copy(); break;
        case CH_F6: paste(); break;
        case CH_F7: switch_to_console_screen();
                    edit_movie();
                    redraw_edit_screen();
                    show_cursor();
                    break;
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

