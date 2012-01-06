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

#include "disk.h"
#include "music.h"
#include "rle.h"
#include "player.h"

#define VIDEO_BASE (unsigned char*)0x8000u

/* RODATA - $7fff: rle buffer
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
    cprintf("%5u", movie.duration[row]);
    revers(0);
    cclear(1);
    revers(1);
    // Prints speed.
    update_color(2, row);
    gotox(SPEED_X);
    cprintf("%3i", movie.speed[row]);
    revers(0);
}

static void draw_fields() {
    unsigned char file_it;
    for (file_it = 0; file_it < FILE_COUNT; ++file_it) {
        draw_row(file_it);
    }
}

static void show_screen() {
    clrscr();
    draw_headers();
    draw_fields();
}

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
        movie.speed[file_it] = 0;
    }
    load_movie();
    inited = 1;
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
            cscanf("%8s", &movie.filename[selected_file]);
            break;
        case 1:  // Duration.
            gotox(DURATION_X);
            cclear(5);
            gotox(DURATION_X);
            cscanf("%5u", &movie.duration[selected_file]);
            break;
        case 2:  // Speed.
            gotox(SPEED_X);
            cclear(3);
            gotox(SPEED_X);
            {
                unsigned int x;
                cscanf("%3u", &x);
                movie.speed[selected_file] = (x & 0xff00u) ? 0xff : x;
            }
            break;
    }
    draw_fields();
}

static unsigned int skip_music_frames(unsigned char file) {
    unsigned int frames = 0;
    unsigned char file_it;
    for (file_it = 0; file_it < file; ++file_it) {
        frames += movie.duration[file];
    }
    return frames;
}

static char packed_anims_valid;

void invalidate_packed_anims() {
    packed_anims_valid = 0;
}

/* The following two are defined by the linker. */
extern unsigned char _RODATA_RUN__;
extern unsigned char _RODATA_SIZE__;
extern unsigned char _STACKSIZE__;
#define RLE_BUFFER (unsigned char*)(((unsigned)&_RODATA_RUN__) + ((unsigned)&_RODATA_SIZE__) + ((unsigned)&_STACKSIZE__))

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
    }
    packed_anims_valid = 1;
}

// Returns 1 if load succeeded, otherwise 0.
static unsigned char unpack_anim(unsigned char file_it, unsigned char alt_screen) {
    const unsigned char* rle_data = movie.start[file_it];
    if (rle_data == NULL) {
        return 0;
    }
    rle_unpack((unsigned char*)(alt_screen ? 0xa000u : 0x8000u), rle_data);
    return 1;
}

void run_anims(unsigned char file_it) {
    unsigned int frameskip_it = skip_music_frames(file_it);
    unsigned int wait_duration = 0;
    unsigned char alt_screen = 0;
    init_music();
    while (frameskip_it--) {
        tick_music();
    }
    init_play();
    for (;;) {
        if (!unpack_anim(file_it, alt_screen)) {
            if (file_it == 0) {
                break;
            } else {
                file_it = 0;
                continue;
            }
        }
        if (wait_anim(wait_duration)) {
            break;
        }
        play_anim(movie.speed[file_it], alt_screen);
        wait_duration = movie.duration[file_it];

        ++file_it;
        file_it %= FILE_COUNT;
        alt_screen ^= 1;
    }
    exit_play();
    *(char*)0xdd00 = 0x17;  // Use graphics bank 0. ($0000-$3fff)
    *(char*)0xd018 = 0x14;  // Point video to 0x400.
    *(char*)0xd020 = 0;
    *(char*)0xd021 = 0;
    show_screen();
}

static void load_music() {
    FILE* f = prompt_open("music", "r");

#define MUSIC_START ((char*)0x1000)
#define MUSIC_STOP ((char*)0x2800)
    fread(MUSIC_START, 1, MUSIC_STOP - MUSIC_START, f);
    fclose(f);

    show_screen();
}

static void save_onefiler() {
    FILE* f = prompt_open("demo", "w");
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
}

static char handle_key(unsigned char key) {
    switch (key) {
        default:
            if (key >= '0' && key <= '9' && selected_column > 0) {
                edit_field();
            }
            break;
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
            printf("save...");
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
            run_anims(selected_file);
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
            pack_anims();
            unpack_anim(selected_file, 0);
            break;
        }
    }
}
