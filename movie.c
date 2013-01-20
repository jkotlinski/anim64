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

#include "movie.h"

#include <conio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "cc65/mycbm.h"

#include "anim.h"
#include "anim_play.h"
#include "disk.h"
#include "music.h"
#include "rle.h"
#include "player.h"
#include "screen.h"

#define VIDEO_BASE (unsigned char*)0x8000u

#pragma bssseg(push, "EDITCODE")
char filename[FILE_COUNT][FILENAME_LENGTH];
char music_path[FILENAME_LENGTH];
#pragma bssseg(pop)

#pragma bssseg (push,"DATA")
/* These variables are used in onefiler mode. They are put in DATA segment
 * instead of BSS to keep them from being zero-initialized.
 */
Movie movie;
#pragma bssseg (pop)

static char selected_file;


/* 0 = file name
 * 1 = duration
 * 2 = speed */
#define MAX_COLUMN 2
static char selected_column;

#define DURATION_X (FILENAME_LENGTH + 1) 
#define SPEED_X (DURATION_X + 4)

#define MOVIE_FILE ".movie,u"

#pragma codeseg("EDITCODE")

static void load_music() {
#define MUSIC_START ((char*)0x1000)
#define MUSIC_STOP ((char*)0x2800)
    char buf[2];
    if (!mycbm_read(MY_LFN, buf, sizeof(buf))) {
        cputs("err");
        cgetc();
        return;
    }
    if (buf[0] == (char)MUSIC_START &&
            buf[1] == ((char)((int)MUSIC_START >> 8))) {
        mycbm_read(MY_LFN, MUSIC_START, MUSIC_STOP - MUSIC_START);
    }
    mycbm_close(MY_LFN);
}

static void load_movie() {
    if (0 != mycbm_open(MY_LFN, 8, CBM_READ, MOVIE_FILE)) {
        return;
    }
    mycbm_read(MY_LFN, filename, sizeof(filename));
    mycbm_read(MY_LFN, &movie, sizeof(movie));
    mycbm_read(MY_LFN, music_path, sizeof(music_path));
    mycbm_close(MY_LFN);
    if (*music_path) {
        if (0 == mycbm_open(MY_LFN, 8, CBM_READ, music_path)) {
            load_music();
        }
    }
}

static void save_movie() {
    mycbm_open(MY_LFN, 8, CBM_WRITE, "@0:" MOVIE_FILE);
    gotoxy(25, 0);
    cputs("save...");
    mycbm_write(MY_LFN, &filename, sizeof(filename));
    mycbm_write(MY_LFN, &movie, sizeof(movie));
    mycbm_write(MY_LFN, music_path, sizeof(music_path));
    mycbm_close(MY_LFN);
    cputs(_oserror ? "err" : "ok");
}

static unsigned char loaded_anim = -1;

static unsigned int skip_music_frames() {
    unsigned int frames = 0;
    unsigned char file_it;
    for (file_it = 0; file_it < selected_file; ++file_it) {
        frames += movie.frames[file_it] * movie.speed[file_it];
    }
    return frames;
}

extern unsigned char _EDITRAM_LAST__;  /* Defined by linker. */

static void draw_headers() {
    textcolor(COLOR_BLUE);
    gotoxy(0, 0);
    cputs("file");
    gotoxy(DURATION_X, 0);
    cputs("dur");
    gotoxy(SPEED_X, 0);
    cputs("spd");
}

static unsigned char update_color(unsigned char column, unsigned char row) {
    const unsigned char color = (row != selected_file || column != selected_column)
        ? (row & 1) ? COLOR_GRAY1 : COLOR_GRAY2
        : COLOR_GREEN;
    textcolor(color);
    return color;
}

static void print_digits(unsigned int number) {
    unsigned char buf[5];
    unsigned char it;
    unsigned char filler = ' ';
    unsigned char digits = 3;
    for (it = 0; it < 3; ++it) {
        buf[it] = number % 10;
        number /= 10;
    }
    while (digits--) {
        char digit = buf[digits];
        if (digit || !digits) {
            cputc('0' + digit);
            filler = '0';
        } else {
            cputc(filler);
        }
    }
}

static void draw_row(unsigned char row) {
    const char y = row + 1;
    const unsigned int offs = y * 40;
    // Prints file name.
    const char color = update_color(0, row);
    revers(1);
    memset((char*)(0xd800 + offs), color, FILENAME_LENGTH);
    memset((char*)(0x400 + offs), 0xa0, FILENAME_LENGTH);
    gotoxy(0, y);
    cputs(filename[row]);
    gotox(DURATION_X - 1);
    revers(0);
    cclear(1);
    revers(1);
    // Prints duration.
    update_color(1, row);
    gotox(DURATION_X);
    print_digits(movie.frames[row]);
    revers(0);
    cclear(1);
    revers(1);
    // Prints speed.
    update_color(2, row);
    gotox(SPEED_X);
    print_digits(movie.speed[row]);
    revers(0);
}

static void draw_fields() {
    unsigned char file_it;
    for (file_it = 0; file_it < FILE_COUNT; ++file_it) {
        draw_row(file_it);
    }
}

static void show_screen() {
    *(char*)0xdd00 = 0x17;  // Use graphics bank 0. ($0000-$3fff)
    *(char*)0xd018 = 0x14;  // Point video to 0x400.
    *(char*)0xd021 = COLOR_BLACK;
    memset((char*)0xd800, COLOR_YELLOW, 0x400);
    clrscr();
    draw_headers();
    draw_fields();
}

static void prompt_music() {
    if (prompt_open("music", CBM_READ, TYPE_PRG)) {
        strcpy(music_path, prompt_path);
        load_music();
    }
    show_screen();
}

static void get_filename() {
    char* const ptr = filename[selected_file];
    char i = 0;
    while (ptr[i]) {
        cputc(ptr[i++]);
    }
    for (;;) {
        unsigned char c = cgetc();
        if (c == CH_ENTER) {
            ptr[i] = 0;
            return;
        }
        if (c == CH_DEL) {
            if (i > 0) {
                --i;
                gotox(wherex() - 1);
                cputc(' ');
                gotox(wherex() - 1);
            }
            continue;
        }
        if (i < FILENAME_LENGTH - 1) {
            cputc(c);
            ptr[i++] = c;
        }
    }
}

unsigned int read_digits() {
    unsigned int number = 0;
    unsigned char digits = 3;
    while (digits > 0) {
        char c = cgetc();
        if (c >= '0' && c <= '9') {
            cputc(c);
            number *= 10;
            number += c - '0';
        } else if (c == CH_ENTER) {
            break;
        }
    }
    return number;
}

static void init() {
    char file_it;
    static char inited;
    if (inited) {
        return;
    }
    /* Since movie is not in BSS, zero-init filename and speed explicitly. */
    for (file_it = 0; file_it < FILE_COUNT; ++file_it) {
        movie.frames[file_it] = 4;
        movie.speed[file_it] = 32;
    }
    load_movie();
    inited = 1;
}

void invalidate_loaded_anim() {
    loaded_anim = -1;
}

static void edit_field() {
    gotoy(selected_file + 1);
    revers(1);
    cursor(1);
    textcolor(COLOR_RED);
    switch (selected_column) {
        case 0:  // File.
            gotox(0);
            cclear(FILENAME_LENGTH);
            gotox(0);
            get_filename();
            invalidate_loaded_anim();
            break;
        case 1:  // Duration.
            gotox(DURATION_X);
            cclear(3);
            gotox(DURATION_X);
            movie.frames[selected_file] = read_digits();
            break;
        case 2:  // Speed.
            gotox(SPEED_X);
            cclear(3);
            gotox(SPEED_X);
            {
                unsigned int x = read_digits();
                if (!x) x = 1;
                if (x & 0xff00u) x = 0xff;
                movie.speed[selected_file] = x;
            }
            break;
    }
    draw_fields();
}

static char* dos_path(unsigned char file) {
    char path[16];
    strcpy(path, filename[file]);
    strcat(path, ",u");
    return path;
}

static int get_file_length(unsigned char file) {
    int length;
    if (!filename[file][0]) {
        return 0;
    }
    /* TODO: Would be great to change to mycbm_load - but then, anim files
     * also must include load address.
     */
    mycbm_open(8, 8, 8, dos_path(file));
    length = mycbm_read(8, &_EDITRAM_LAST__, (char*)0x8000 - &_EDITRAM_LAST__);

    mycbm_close(8);
    return length;
}

static void write_onefiler_anims() {
    unsigned int heap_start = (unsigned int)HEAP_START;
    static const unsigned int heap_end = 0xd000u;
    unsigned char file_it;
    for (file_it = 0; file_it < FILE_COUNT; ++file_it) {
        const unsigned int file_length = get_file_length(file_it);
        if (!file_length) {
            if (*filename[file_it]) {
                mycbm_close(MY_LFN);
                textcolor(COLOR_RED);
                gotoxy(0, file_it + 2);
                cputs(filename[file_it]);
                cputs(" read err");
                return;
            }
            continue;
        }
        gotoxy(0, file_it + 2);
        cputs(filename[file_it]);
        cputs(": ");
        cputhex16(file_length);
        if (_EDITRAM_LAST__ != 2) {  // Checks that anim version == 2.
            mycbm_close(MY_LFN);
            textcolor(COLOR_RED);
            cputs(" version err");
            return;
        }
        // Enough space left? +2 for size, another +2 for EOF marker.
        if (heap_end - heap_start >= file_length + 2 + 2) {
            const unsigned int next_addr = heap_start + 2 + file_length;
            if (mycbm_write(MY_LFN, &next_addr, sizeof(next_addr)) <= 0 ||
                    mycbm_write(MY_LFN, &_EDITRAM_LAST__, file_length) <= 0) {
                mycbm_close(MY_LFN);
                textcolor(COLOR_RED);
                cputs(" disk full?");
                return;
            }
            heap_start += file_length + sizeof(file_length);
        } else {
            mycbm_close(MY_LFN);
            textcolor(COLOR_RED);
            cputs(" out of mem");
            return;
        }
    }
    // End of file marker (0).
    heap_start = 0;
    mycbm_write(MY_LFN, &heap_start, sizeof(heap_start));
    mycbm_close(MY_LFN);
    cputs(" ok!");
}

static void save_onefiler() {
    char buf[2] = { 1, 8 };
    if (prompt_open("demo", CBM_WRITE, TYPE_PRG) == NULL) {
        return;
    }
    cputs("write...");
    if (mycbm_write(MY_LFN, buf, sizeof(buf)) <= 0 ||
            mycbm_write(MY_LFN, (char*)0x801, (unsigned int)HEAP_START - 0x801) <= 0) {
        mycbm_close(MY_LFN);
        textcolor(COLOR_RED);
        cputs(" disk full?");
        cgetc();
        return;
    }
    *(char*)0xd018 |= 2;  // lower/uppercase gfx
    write_onefiler_anims();
    cgetc();
    *(char*)0xd018 &= ~2;  // uppercase + gfx
}

static char load_selected_anim() {
    // Loads and unpacks selected movie, returns to animation editor.
    if (loaded_anim == selected_file) return 1; 
    if (mycbm_open(MY_LFN, 8, CBM_READ, dos_path(selected_file))) {
        return 0;  // Open error.
    }
    if (!load_and_unpack_anim()) {
        return 0;
    }
    loaded_anim = selected_file;
    return 1;
}

static char handle_key(unsigned char key) {
    switch (key) {
        case CH_CURS_DOWN:
            if (selected_file < FILE_COUNT - 1) {
                ++selected_file;
                draw_row(selected_file - 1);
                draw_row(selected_file);
            }
            break;
        case CH_CURS_UP:
            if (selected_file) {
                --selected_file;
                draw_row(selected_file + 1);
                draw_row(selected_file);
            }
            break;
        case CH_CURS_RIGHT:
            if (selected_column < MAX_COLUMN) {
                ++selected_column;
                draw_row(selected_file);
            }
            break;
        case CH_CURS_LEFT:
            if (selected_column) {
                --selected_column;
                draw_row(selected_file);
            }
            break;
        case CH_ENTER:
            edit_field();
            break;
        case CH_F1: load_movie(); break;
        case CH_F2:
            save_movie();
            break;
        case CH_F3: prompt_music(); break;
        case CH_F5:
            save_onefiler();
            show_screen();
            break;
        case CH_STOP:
            if (!load_selected_anim()) {
                textcolor(COLOR_RED);
                cputs("err");
                break;
            }
            preview_play_anim(movie.speed[selected_file],
                    skip_music_frames(),
                    movie.frames[selected_file]);
            show_screen();
            break;
        case CH_F7:  // Go to animation editor.
            return 1;
    }
    return 0;
}

void edit_movie() {
    init();
    show_screen();

    for (;;) {
        if (kbhit() && handle_key(cgetc())) {
            load_selected_anim();
            return;
        }
    }
}
