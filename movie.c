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

#include "diff.h"
#include "disk.h"
#include "music.h"
#include "rle.h"
#include "player.h"

#define VIDEO_BASE (unsigned char*)0x8000u

/* RAM end - $7fff: rle buffer
 * $8000 - $8fff: screen 0-3, + border/screen color
 * $9000 - $9fff: color 0-3
 * $a000 - $afff: screen 4-7, + border/screen color
 * $b000 - $bfff: color 4-7
 * $c000 - $cfff: unused
 * $e000 - $ffff: unused
 */

#define FILE_COUNT 24
#define FILENAME_LENGTH 8
#pragma bssseg (push,"DATA")
/* These variables are used in onefiler mode. They are put in DATA segment
 * instead of BSS to keep them from being zero-initialized.
 */
static struct Movie {
    char filename[FILE_COUNT][FILENAME_LENGTH];
    unsigned int duration[FILE_COUNT];
    unsigned char speed[FILE_COUNT];
    unsigned char* start[FILE_COUNT];
} movie;
static char is_onefiler_marker[4];
#pragma bssseg (pop)

static const char onefiler_magic[4] = { 'p', 'l', 'a', 'y' };

char is_onefiler() {
    return !memcmp(is_onefiler_marker, onefiler_magic, sizeof(onefiler_magic));
}

static char selected_file;


/* 0 = file name
 * 1 = duration
 * 2 = speed */
#define MAX_COLUMN 2
static char selected_column;

#define DURATION_X (FILENAME_LENGTH + 1) 
#define SPEED_X (DURATION_X + 6)

static const char* MOVIE_FILE = ".movie";

static void load_movie() {
    FILE* f = fopen(MOVIE_FILE, "r");
    if (!f) return;
    fread(&movie, sizeof(movie), 1, f);
    fclose(f);
}

static void save_movie() {
    FILE* f = fopen(MOVIE_FILE, "w");
    if (!f || !fwrite(&movie, sizeof(movie), 1, f)) {
        puts("err");
    } else {
        puts("ok");
    }
    fclose(f);
}

static char packed_anims_valid;

static unsigned int skip_music_frames() {
    unsigned int frames = 0;
    unsigned char file_it;
    for (file_it = 0; file_it < selected_file; ++file_it) {
        frames += movie.duration[file_it];
    }
    return frames;
}

/* The following two are defined by the linker. */
extern unsigned char _EDITRAM_START__;
extern unsigned char _EDITRAM_SIZE__;
#define RLE_BUFFER (unsigned char*)(((unsigned)&_EDITRAM_START__) + ((unsigned)&_EDITRAM_SIZE__))

// Returns 1 if load succeeded, otherwise 0.
static unsigned char unpack_anim(unsigned char file_it, unsigned char alt_screen) {
    const unsigned char* rle_data = movie.start[file_it];
    unsigned char* screen_base = (unsigned char*)(alt_screen ? 0xa000u : 0x8000u);
    if (rle_data == NULL) {
        return 0;
    }
    rle_unpack(screen_base, rle_data);
    undiff(screen_base);
    return 1;
}

void show_screen();

extern volatile unsigned char caught_irqs;

void play_movie() {
    unsigned int wait_duration = 0;
    unsigned char file_it = 0;
    unsigned char alt_screen = 0;
    init_music();
    init_play();
    for (;;) {
        unpack_anim(file_it, alt_screen);
        while (wait_duration--) {
            while (!caught_irqs) {}
            --caught_irqs;
        }
        play_anim(movie.speed[file_it], alt_screen);
        wait_duration = movie.duration[file_it];

        ++file_it;
        file_it %= FILE_COUNT;
        alt_screen ^= 1;
    }
}

static void load_music() {
    FILE* f = prompt_open("music", "r");

#define MUSIC_START ((char*)0x1000)
#define MUSIC_STOP ((char*)0x2800)
    if ((char)fgetc(f) == (char)MUSIC_START &&
            (char)fgetc(f) == ((char)((int)MUSIC_START >> 8))) {
        fread(MUSIC_START, 1, MUSIC_STOP - MUSIC_START, f);
    }
    fclose(f);

    show_screen();
}

#pragma codeseg("EDITCODE")

static void read_filename() {
    char* ptr = movie.filename[selected_file];
    char chars = 7;
    while (chars-- > 0) {
        unsigned char c = cgetc();
        if (c == CH_ENTER) {
            break;
        }
        cputc(c);
        *ptr = c;
        ++ptr;
    }
    *ptr = 0;
}

static unsigned int read_digits(unsigned digits) {
    unsigned int number = 0;
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
static unsigned char update_color(unsigned char column, unsigned char row) {
    const unsigned char color = (row != selected_file || column != selected_column)
        ? (row & 1) ? COLOR_GRAY1 : COLOR_GRAY2
        : COLOR_GREEN;
    textcolor(color);
    return color;
}

static void draw_headers() {
    textcolor(COLOR_BLUE);
    gotoxy(0, 0);
    cputs("file");
    gotoxy(DURATION_X, 0);
    cputs("dur.");
    gotoxy(SPEED_X, 0);
    cputs("spd");
}

static void print_digits(unsigned int number, char digits) {
    char buf[5];
    char it;
    char filler = ' ';
    for (it = 0; it < digits; ++it) {
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
    cputs(movie.filename[row]);
    gotox(DURATION_X - 1);
    revers(0);
    cclear(1);
    revers(1);
    // Prints duration.
    update_color(1, row);
    gotox(DURATION_X);
    print_digits(movie.duration[row], 5);
    revers(0);
    cclear(1);
    revers(1);
    // Prints speed.
    update_color(2, row);
    gotox(SPEED_X);
    print_digits(movie.speed[row], 3);
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

static void init() {
    char file_it;
    static char inited;
    if (inited || is_onefiler()) {
        return;
    }
    /* Since movie is not in BSS, zero-init filename and speed explicitly. */
    for (file_it = 0; file_it < FILE_COUNT; ++file_it) {
        movie.filename[file_it][0] = '\0';
        movie.duration[file_it] = 100;
        movie.speed[file_it] = 1;
    }
    load_movie();
    inited = 1;
}

void invalidate_packed_anims() {
    packed_anims_valid = 0;
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
            read_filename();
            invalidate_packed_anims();
            break;
        case 1:  // Duration.
            gotox(DURATION_X);
            cclear(5);
            gotox(DURATION_X);
            movie.duration[selected_file] = read_digits(5);
            break;
        case 2:  // Speed.
            gotox(SPEED_X);
            cclear(3);
            gotox(SPEED_X);
            {
                unsigned int x = read_digits(3);
                if (!x) x = 1;
                if (x & 0xff00u) x = 0xff;
                movie.speed[selected_file] = x;
            }
            break;
    }
    draw_fields();
}


/* Packs the different anims into RLE buffer. */
void pack_anims() {
    unsigned char* rle_ptr = RLE_BUFFER;
    unsigned char anim_it;
    if (packed_anims_valid) {
        return;
    }
    for (anim_it = 0; anim_it < FILE_COUNT; ++anim_it) {
        FILE* f;
        movie.start[anim_it] = NULL;
        if (!movie.filename[anim_it][0]) {
            continue;
        }
        f = fopen(movie.filename[anim_it], "r");
        if (!f) {
            continue;
        }
        movie.start[anim_it] = rle_ptr;
        rle_ptr += fread(rle_ptr, 1, 0x8000u - (unsigned int)RLE_BUFFER, f);
        fclose(f);
        if ((unsigned int)rle_ptr > 0x8000u) {
            puts("out of mem");
            while (1);
        }
    }
    packed_anims_valid = 1;
}

static void save_onefiler() {
    FILE* f;
    _filetype = 'p';  // .prg
    f = prompt_open("demo", "w");
    if (f == NULL) {
        return;
    }
    memcpy(is_onefiler_marker, onefiler_magic, sizeof(onefiler_magic));
    // Writes load address.
    fputc(1, f);
    fputc(8, f);
    // Saves $801 - $7fff.
    fwrite((char*)0x801, 0x7fff - 0x801, 1, f);
    *is_onefiler_marker = 0;
    if (EOF == fclose(f)) {
        textcolor(COLOR_RED);
        puts("disk full?");
        cgetc();
    }
    _filetype = 'u';  // Switch back to .usr
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
            gotoxy(20, 0);
            cputs("save...");
            save_movie();
            break;
        case CH_F3: load_music(); break;
        case CH_F5:
            pack_anims();
            save_onefiler();
            show_screen();
            break;
        case CH_STOP:
            pack_anims();
            unpack_anim(selected_file, 1);
            skip_music_frames();
            init_play();
            play_anim(32, 1);
            wait_anim(movie.duration[selected_file]);
            exit_play();
            show_screen();
            break;
        case CH_F7:  // Go to animation editor.
            pack_anims();
            unpack_anim(selected_file, 0);
            return 1;
    }
    return 0;
}

void edit_movie() {
    init();
    show_screen();

    for (;;) {
        if (kbhit() && handle_key(cgetc())) {
            pack_anims();
            unpack_anim(selected_file, 0);
            break;
        }
    }
}
